#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MultiPlayerLevel.h"

#include <cmath>
#include <numbers>

#include "../core/Everstead.h"
#include "../core/FrameProfiler.h"
#include "../entity/player/MultiPlayerLocalPlayer.h"
#include "../entity/player/RemotePlayer.h"
#include "../sound/SoundEngine.h"
#include "../sound/SoundRegistry.h"
#include "../utils/Time.h"
#include "../utils/math/Mth.h"
#include "LevelRenderer.h"
#include "block/Block.h"
#include "chunk/Chunk.h"
#include "particle/ParticleEngine.h"

#include <glm/geometric.hpp>

MultiPlayerLevel::MultiPlayerLevel()
{
    m_levelRenderer  = nullptr;
    m_particleEngine = std::make_unique<ParticleEngine>(this);
    m_spawnScratch   = {};
    m_rainSoundTime  = 0;
}

void MultiPlayerLevel::SetLevelRenderer(LevelRenderer *levelRenderer)
{
    m_levelRenderer = levelRenderer;
}

void MultiPlayerLevel::Update(float partialTicks)
{
    {
        FrameProfilerScope scope("Level::UpdateChunks");
        UpdateChunks(Everstead::GetInstance()->options.chunkIntakePerTick);
        UpdateUrgentMeshes();
    }

    {
        FrameProfilerScope scope("Level::UpdateLighting");
        UpdateLighting(400);
    }

    {
        FrameProfilerScope scope("Level::UpdateMeshes");
        UpdateMeshes();
    }

    {
        FrameProfilerScope scope("Level::UpdateParticles");
        m_particleEngine->Tick(Time::GetDelta());
    }

    Everstead::GetInstance()->GetLocalPlayer()->Update(partialTicks);
}

void MultiPlayerLevel::UpdateUrgentMeshes()
{
    const int urgentBudget    = 8;
    const int immediateBudget = 4;

    bool scheduledRebuilds = false;
    int immediate          = 0;
    for (int i = 0; i < urgentBudget;)
    {
        std::vector<SectionPos> sections;
        if (PollUrgentDirtySections(&sections))
        {
            i += (int) sections.size();
            if (immediate + (int) sections.size() <= immediateBudget && Everstead::GetInstance()->options.instantBlockUpdates)
            {
                immediate += (int) sections.size();
                for (size_t j = 0; j < sections.size(); j++)
                {
                    m_levelRenderer->RebuildSectionNow(sections[j]);
                }
            }
            else
            {
                m_levelRenderer->RebuildSectionsUrgentSynchronized(sections);
                scheduledRebuilds = true;
            }

            continue;
        }

        ChunkPos pos;
        if (!PollUrgentDirtyChunk(&pos))
        {
            break;
        }

        m_levelRenderer->RebuildChunkDeferred(pos, true);
        scheduledRebuilds = true;
        i++;
    }

    if (scheduledRebuilds)
    {
        m_levelRenderer->FlushMeshScheduling();
    }
}

void MultiPlayerLevel::UpdateMeshes()
{
    FlushMeshWaitingChunks();

    const int meshBudget = 16;
    ChunkPos center      = GetPriorityChunk();

    bool scheduledRebuilds = false;
    for (int i = 0; i < meshBudget; i++)
    {
        ChunkPos pos;
        if (!PollDirtyChunk(&pos, center))
        {
            break;
        }

        m_levelRenderer->RebuildChunkDeferred(pos, false);
        scheduledRebuilds = true;
    }

    if (scheduledRebuilds)
    {
        m_levelRenderer->FlushMeshScheduling();
    }
}

void MultiPlayerLevel::FlushMeshWaitingChunks()
{
    // Chunks on the render distance edge never get every neighbor, so they mesh after a short wait anyway
    const std::chrono::milliseconds maxWait(1000);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    for (auto waitingIt = m_meshWaitingChunks.begin(); waitingIt != m_meshWaitingChunks.end();)
    {
        const ChunkPos &pos = waitingIt->first;
        if (!HasChunk(pos))
        {
            waitingIt = m_meshWaitingChunks.erase(waitingIt);
            continue;
        }

        if (now - waitingIt->second < maxWait && !IsChunkReadyToMesh(pos))
        {
            waitingIt++;
            continue;
        }

        MarkChunkDirty(BlockPos(pos.x * Chunk::s_sizeX, 0, pos.z * Chunk::s_sizeZ));
        waitingIt = m_meshWaitingChunks.erase(waitingIt);
    }
}

bool MultiPlayerLevel::IsChunkReadyToMesh(const ChunkPos &pos) const
{
    if (IsSkyLightSeamQueued(pos))
    {
        return false;
    }

    const int neighborOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int i = 0; i < 4; i++)
    {
        ChunkPos neighborPos(pos.x + neighborOffsets[i][0], pos.z + neighborOffsets[i][1]);
        if (!HasChunk(neighborPos) || IsSkyLightSeamQueued(neighborPos))
        {
            return false;
        }
    }

    return true;
}

ChunkPos MultiPlayerLevel::GetPriorityChunk() const
{
    const glm::dvec3 &position = Everstead::GetInstance()->GetCamera()->GetPosition();
    return ChunkPos(Mth::FloorDiv((int) floor(position.x), Chunk::s_sizeX), Mth::FloorDiv((int) floor(position.z), Chunk::s_sizeZ));
}

void MultiPlayerLevel::ReceiveChunk(const ChunkPos &pos, std::unique_ptr<Chunk> chunk)
{
    if (!AdoptChunk(pos, std::move(chunk)))
    {
        return;
    }

    glm::dvec3 playerPos    = Everstead::GetInstance()->GetLocalPlayer()->GetPosition();
    int playerChunkX        = Mth::FloorDiv((int) floor(playerPos.x), Chunk::s_sizeX);
    int playerChunkZ        = Mth::FloorDiv((int) floor(playerPos.z), Chunk::s_sizeZ);
    int playerChunkDeltaX   = pos.x - playerChunkX;
    int playerChunkDeltaZ   = pos.z - playerChunkZ;
    int playerChunkDistance = playerChunkDeltaX * playerChunkDeltaX + playerChunkDeltaZ * playerChunkDeltaZ;
    if (playerChunkDistance <= 4)
    {
        MarkChunkDirtyUrgent(pos);
    }
    else
    {
        m_meshWaitingChunks[pos] = std::chrono::steady_clock::now();
    }

    // Neighbors that are still waiting will mesh with this chunk's edge once they are ready
    const int neighborOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int i = 0; i < 4; i++)
    {
        ChunkPos neighborPos(pos.x + neighborOffsets[i][0], pos.z + neighborOffsets[i][1]);
        if (HasChunk(neighborPos) && m_meshWaitingChunks.find(neighborPos) == m_meshWaitingChunks.end())
        {
            MarkChunkDirty(BlockPos(neighborPos.x * Chunk::s_sizeX, 0, neighborPos.z * Chunk::s_sizeZ));
        }
    }
}

ParticleEngine *MultiPlayerLevel::GetParticleEngine() const
{
    return m_particleEngine.get();
}

void MultiPlayerLevel::SpawnParticle(const ParticleSpawnParams &params)
{
    m_particleEngine->Spawn(params);
}

void MultiPlayerLevel::PlaySound(const ResourceLocation &location, const glm::dvec3 &position, float volume, float pitch)
{
    SoundEngine *soundEngine = Everstead::GetInstance()->GetSoundEngine();
    SoundEvent *sound        = SoundRegistry::ByName(location);
    if (soundEngine && sound)
    {
        soundEngine->PlaySound3D(sound, position, volume, pitch);
    }
}

void MultiPlayerLevel::PlayThunder(const glm::dvec3 &position, float volume, float pitch)
{
    const double blocksPerTick = 17.0;

    double distance = glm::distance(position, Everstead::GetInstance()->GetLocalPlayer()->GetPosition());
    m_pendingThunder.push_back({(int) (distance / blocksPerTick), volume, pitch});
}

void MultiPlayerLevel::PlaySound(const ResourceLocation &location, float volume, float pitch)
{
    SoundEngine *soundEngine = Everstead::GetInstance()->GetSoundEngine();
    SoundEvent *sound        = SoundRegistry::ByName(location);
    if (soundEngine && sound)
    {
        soundEngine->PlaySound(sound, volume, pitch);
    }
}

void MultiPlayerLevel::Tick()
{
    if (GetTickRateManager()->RunsNormally())
    {
        TickTime();
    }

    Level::Tick();

    FrameProfilerScope scope("Level.AnimateTick");
    AnimateTick();
}

TickRateManager *MultiPlayerLevel::GetTickRateManager() const
{
    return Everstead::GetInstance()->GetTickRateManager();
}

void MultiPlayerLevel::AdoptBlockEntities(const ChunkPos &pos)
{
    RemoveBlockEntities(pos);

    const Chunk *chunk = GetChunk(pos);
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        if (chunk->IsSectionEmpty(section))
        {
            continue;
        }

        for (int y = section * ChunkSection::s_size; y < (section + 1) * ChunkSection::s_size; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int x = 0; x < Chunk::s_sizeX; x++)
                {
                    Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                    if (block && block->GetBlockEntityId())
                    {
                        GetBlockEntity(BlockPos(pos.x * Chunk::s_sizeX + x, y, pos.z * Chunk::s_sizeZ + z));
                    }
                }
            }
        }
    }
}

void MultiPlayerLevel::TickWeather()
{
    Level::TickWeather();
    TickRain();
    TickThunder();
}

void MultiPlayerLevel::AnimateTick()
{
    const int range             = 16;
    const int samples           = 1000;
    const int voidParticleY     = 8;
    const int waterSoundChance  = 64;
    const int lavaPopChance     = 100;
    const int lavaAmbientChance = 200;
    const int fireSoundChance   = 24;

    const glm::dvec3 &playerPos = Everstead::GetInstance()->GetLocalPlayer()->GetPosition();
    BlockPos center((int) floor(playerPos.x), (int) floor(playerPos.y), (int) floor(playerPos.z));
    bool nearVoid                 = center.y - range < voidParticleY;
    Block *resonantCrystal        = Block::ByName("resonant_crystal");
    Block *richResonantCrystal    = Block::ByName("rich_resonant_crystal");
    SoundEvent *resonantHum       = SoundRegistry::ByName(ResourceLocation("ambient.resonant_hum"));
    Block *resonantSocket         = Block::ByName("resonant_socket");
    SoundEvent *resonantSocketHum = SoundRegistry::ByName(ResourceLocation("ambient.resonant_socket_hum"));

    m_spawnScratch.clear();
    for (int i = 0; i < samples; i++)
    {
        int x = center.x + m_random.NextInt(0, range - 1) - m_random.NextInt(0, range - 1);
        int y = center.y + m_random.NextInt(0, range - 1) - m_random.NextInt(0, range - 1);
        int z = center.z + m_random.NextInt(0, range - 1) - m_random.NextInt(0, range - 1);
        if (y < 0 || y >= Chunk::s_sizeY)
        {
            continue;
        }

        BlockPos pos(x, y, z);
        uint32_t blockId = GetBlockId(pos);
        if (blockId == 0)
        {
            if (nearVoid && m_random.NextInt(0, voidParticleY - 1) > y)
            {
                ParticleSpawnParams params;
                params.type     = eParticleType_DepthSuspend;
                params.position = glm::dvec3(x + m_random.NextDouble(), y + m_random.NextDouble(), z + m_random.NextDouble());
                m_spawnScratch.push_back(params);
            }

            continue;
        }

        Block *block = Block::ById(blockId);
        block->AnimateTick(this, pos, &m_random);
        if (block == resonantCrystal || block == richResonantCrystal)
        {
            Everstead::GetInstance()->GetSoundEngine()->PlayLoop3D(resonantHum, glm::dvec3(x + 0.5, y + 0.5, z + 0.5), 1.0f);
            continue;
        }

        if (block == resonantSocket)
        {
            Everstead::GetInstance()->GetSoundEngine()->PlayLoop3D(resonantSocketHum, glm::dvec3(x + 0.5, y + 0.5, z + 0.5), 1.0f);
            continue;
        }

        if (block->GetRenderShape() == Block::eRenderShape_FIRE)
        {
            if (m_random.NextInt(0, fireSoundChance - 1) == 0)
            {
                PlaySound("fire.fire", glm::dvec3(x + 0.5, y + 0.5, z + 0.5), 1.0f + m_random.NextFloat(), 0.3f + m_random.NextFloat() * 0.7f);
            }

            continue;
        }

        if (block->IsLava())
        {
            if (GetBlockId(BlockPos(x, y + 1, z)) != 0)
            {
                continue;
            }

            if (m_random.NextInt(0, lavaPopChance - 1) == 0)
            {
                PlaySound("liquid.lavapop", glm::dvec3(x + m_random.NextDouble(), y + 1.0, z + m_random.NextDouble()),
                          0.2f + m_random.NextFloat() * 0.2f, 0.9f + m_random.NextFloat() * 0.15f);
            }

            if (m_random.NextInt(0, lavaAmbientChance - 1) == 0)
            {
                PlaySound("liquid.lava", glm::dvec3(x, y, z), 0.2f + m_random.NextFloat() * 0.2f, 0.9f + m_random.NextFloat() * 0.15f);
            }

            continue;
        }

        if (!block->IsLiquid() || m_random.NextInt(0, waterSoundChance - 1) != 0)
        {
            continue;
        }

        uint16_t depth = GetBlockData(pos);
        if (depth > 0 && depth < 8)
        {
            PlaySound("liquid.water", glm::dvec3(x + 0.5, y + 0.5, z + 0.5), m_random.NextFloat() * 0.25f + 0.75f, m_random.NextFloat() + 0.5f);
        }
    }

    if (!m_spawnScratch.empty())
    {
        GetParticleEngine()->Spawn(m_spawnScratch);
    }
}

void MultiPlayerLevel::TickRain()
{
    const int range     = 10;
    const int nearRange = 4;

    float rain = GetRainLevel(1.0f);
    if (rain <= 0.0f)
    {
        return;
    }

    const glm::dvec3 &playerPos = Everstead::GetInstance()->GetLocalPlayer()->GetPosition();
    int playerX                 = (int) floor(playerPos.x);
    int playerY                 = (int) floor(playerPos.y);
    int playerZ                 = (int) floor(playerPos.z);

    m_spawnScratch.clear();
    int dropCount = (int) (100.0f * rain * rain);
    int nearCount = (int) (40.0f * rain * rain);
    int samples   = 0;
    glm::dvec3 soundPos;
    bool soundSnow = false;
    for (int i = 0; i < dropCount + nearCount; i++)
    {
        int spread = i < dropCount ? range : nearRange;
        int x      = playerX + m_random.NextInt(0, spread * 2) - spread;
        int z      = playerZ + m_random.NextInt(0, spread * 2) - spread;
        BlockPos pos(x, GetRainHeight(x, z), z);
        if (pos.y > playerY + range || pos.y < playerY - range)
        {
            continue;
        }

        BlockPos below(x, pos.y - 1, z);
        uint32_t belowId = GetBlockId(below);
        Block *block     = Block::ById(belowId);
        if (belowId == 0 || block->IsLava())
        {
            continue;
        }

        bool snowing = IsSnowingAt(pos);
        if (!snowing && !IsRainingAt(pos))
        {
            continue;
        }

        glm::dvec3 position(x + m_random.NextDouble(), pos.y + 0.1, z + m_random.NextDouble());
        if (!snowing)
        {
            ParticleSpawnParams params;
            params.type     = eParticleType_Rain;
            params.position = position;
            m_spawnScratch.push_back(params);
            if (block->IsLiquid())
            {
                m_levelRenderer->AddRipple(glm::dvec3(position.x, below.y + block->GetLiquidHeight(GetBlockData(below)), position.z));
            }
        }

        samples++;
        if (m_random.NextInt(1, samples) == 1)
        {
            soundPos  = position;
            soundSnow = snowing;
        }
    }

    if (samples == 0)
    {
        return;
    }

    if (!m_spawnScratch.empty())
    {
        GetParticleEngine()->Spawn(m_spawnScratch);
    }

    if (m_random.NextInt(0, 2) >= m_rainSoundTime++)
    {
        return;
    }

    m_rainSoundTime  = 0;
    std::string name = (soundSnow ? "ambient.weather.snow" : "ambient.weather.rain") + std::to_string(m_random.NextInt(1, 4));
    if (!Everstead::GetInstance()->options.soundRaytracing && soundPos.y > playerPos.y + 1.0 && GetRainHeight(playerX, playerZ) > playerY)
    {
        PlaySound(name, soundPos, 0.1f, 0.5f);
    }
    else
    {
        PlaySound(name, soundPos, 0.2f, 1.0f);
    }
}

void MultiPlayerLevel::TickThunder()
{
    const float distantFlashChance = 1.0f / 150.0f;

    for (size_t i = 0; i < m_pendingThunder.size();)
    {
        if (m_pendingThunder[i].ticks > 0)
        {
            m_pendingThunder[i].ticks--;
            i++;
            continue;
        }

        PlaySound("ambient.weather.thunder" + std::to_string(m_random.NextInt(1, 3)), m_pendingThunder[i].volume, m_pendingThunder[i].pitch);
        m_pendingThunder.erase(m_pendingThunder.begin() + i);
    }

    if (GetLightningFlash(0.0f) > 0.0f || m_random.NextFloat() >= GetThunderLevel(1.0f) * distantFlashChance)
    {
        return;
    }

    double angle                = m_random.NextDouble() * std::numbers::pi * 2.0;
    double distance             = 120.0 + m_random.NextDouble() * 230.0;
    const glm::dvec3 &playerPos = Everstead::GetInstance()->GetLocalPlayer()->GetPosition();
    glm::dvec3 position(playerPos.x + cos(angle) * distance, Everstead::GetInstance()->options.cloudHeight, playerPos.z + sin(angle) * distance);
    AddLightningFlash(position, 0.3f + m_random.NextFloat() * 0.3f);
    PlayThunder(position, 0.35f, 0.6f + m_random.NextFloat() * 0.15f);
}

bool MultiPlayerLevel::IsClientSide() const
{
    return true;
}

Player *MultiPlayerLevel::CreatePlayer(const std::wstring &name)
{
    return new RemotePlayer(this, name);
}

Player *MultiPlayerLevel::GetLocalPlayer() const
{
    return Everstead::GetInstance()->GetLocalPlayer();
}

void MultiPlayerLevel::OnBlockLightChanged(const std::unordered_set<SectionPos, SectionPosHash> &sections, bool urgent)
{
    for (const SectionPos &sectionPos : sections)
    {
        m_levelRenderer->RebuildSectionDeferred(sectionPos, urgent);
    }

    m_levelRenderer->FlushMeshScheduling();
}

bool MultiPlayerLevel::IsChunkMeshBusy(const ChunkPos &pos) const
{
    return m_levelRenderer->HasActiveRebuild(pos);
}

void MultiPlayerLevel::ReleaseChunkMeshes(const ChunkPos &pos)
{
    m_levelRenderer->DropChunkMeshes(pos);
}

void MultiPlayerLevel::OnSectionMeshDirty(const SectionPos &pos)
{
    m_levelRenderer->RebuildSectionDeferred(pos, false);
}

bool MultiPlayerLevel::IsClipTarget(const Entity *entity) const
{
    MultiPlayerLocalPlayer *player = Everstead::GetInstance()->GetLocalPlayer();
    return entity->GetUniqueID() != player->GetUniqueID() && entity != player->GetVehicle() && entity->IsPickable();
}

#endif
