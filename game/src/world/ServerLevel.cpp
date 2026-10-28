#include "ServerLevel.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <unordered_set>

#include "../core/EversteadServer.h"
#include "../core/FrameProfiler.h"
#include "../entity/EntityFactory.h"
#include "../entity/EntityTracker.h"
#include "../entity/Mob.h"
#include "../entity/animal/Squid.h"
#include "../entity/global/LightningBolt.h"
#include "../entity/item/Boat.h"
#include "../entity/item/ExperienceOrb.h"
#include "../entity/item/ItemEntity.h"
#include "../entity/item/Minecart.h"
#include "../entity/monster/Monster.h"
#include "../entity/monster/Slime.h"
#include "../entity/player/ServerPlayer.h"
#include "../network/PlayerChunkSender.h"
#include "../network/PlayerList.h"
#include "../network/listener/PlayerConnection.h"
#include "../network/packet/s2c/AddEntityS2CPacket.h"
#include "../network/packet/s2c/AnimateHurtS2CPacket.h"
#include "../network/packet/s2c/EntityEventS2CPacket.h"
#include "../network/packet/s2c/MultiBlockUpdateS2CPacket.h"
#include "../network/packet/s2c/RemoveEntitiesS2CPacket.h"
#include "../network/packet/s2c/SetBoatDataS2CPacket.h"
#include "../network/packet/s2c/SetEntityDataS2CPacket.h"
#include "../network/packet/s2c/SetMobDataS2CPacket.h"
#include "../network/packet/s2c/SetPassengersS2CPacket.h"
#include "../network/packet/s2c/SetWeatherS2CPacket.h"
#include "../network/packet/s2c/SoundS2CPacket.h"
#include "../network/packet/s2c/TakeItemEntityS2CPacket.h"
#include "../tag/CompoundTag.h"
#include "../utils/StringUtils.h"
#include "../utils/math/Mth.h"
#include "block/LeavesBlock.h"
#include "block/SnowLayerBlock.h"
#include "generation/TerrainGenerator.h"

#include <glm/gtx/norm.hpp>

static const double s_itemVoidY          = -64.0;
static const double s_pickupReachXZ      = 1.0;
static const double s_pickupReachY       = 0.5;
static const double s_mergeRadius        = 0.5;
static const int s_mergeCheckInterval    = 10;
static const int s_blockTickLoadedRadius = 8;
static const int s_outOfRangeTickDelay   = 20;

ServerLevel::ServerLevel(EversteadServer *server)
{
    m_server                  = server;
    m_blockChanges            = {};
    m_blockChangeSet          = {};
    m_itemScratch             = {};
    m_removedItemScratch      = {};
    m_removedEntityIds        = {};
    m_simulationCenterScratch = {};
    m_scheduledBlockTicks     = {};
    m_chunkBlockTicks         = {};
    m_scheduledBlockTickCount = 0;
    m_parkedBlockTicks        = {};
    m_nextTickSequence        = 0;
    m_handlingBlockTicks      = false;
    m_blockEvents             = {};
    m_lightningRandomValue    = m_random.NextUInt();
}

void ServerLevel::Tick()
{
    UpdateSimulationCenters();

    {
        FrameProfilerScope scope("Server.UpdateChunks");
        UpdateChunks(EversteadServer::GetInstance()->settings.chunkIntakePerTick);
    }

    bool runsNormally = GetTickRateManager()->RunsNormally();
    // Block ticks and block events run before entities and block entities because piston timing depends on that order
    if (runsNormally)
    {
        FrameProfilerScope scope("Server.BlockTicks");
        TickTime();
        if (GetGameTime() % s_outOfRangeTickDelay == 0)
        {
            UnparkBlockTicks();
        }

        m_handlingBlockTicks = true;
        ProcessScheduledBlockTicks(GetGameTime());
        m_handlingBlockTicks = false;
        RunBlockEvents();
    }

    Level::Tick();

    ClearMeshDirtyQueues();

    {
        FrameProfilerScope scope("Server.BlockChanges");
        BroadcastBlockChanges();
    }

    if (!runsNormally)
    {
        BroadcastRemovedEntities();
        return;
    }

    {
        FrameProfilerScope scope("Server.Items");
        TickItems();
    }

    TickExperienceOrbs();

    if (GetGameRules().GetBool(eGameRule_DoMobSpawning))
    {
        FrameProfilerScope scope("Server.MobSpawning");
        TickMobSpawning();
    }

    BroadcastRemovedEntities();
}

TickRateManager *ServerLevel::GetTickRateManager() const
{
    return m_server->GetTickRateManager();
}

void ServerLevel::TickWeather()
{
    if (GetGameRules().GetBool(eGameRule_DoWeatherCycle))
    {
        if (m_thunderTime <= 0)
        {
            m_thunderTime = m_thundering ? m_random.NextInt(0, 20 * 60 * 10 - 1) + 20 * 60 * 3
                                         : m_random.NextInt(0, WorldProvider::s_dayTicks * 7 - 1) + WorldProvider::s_dayTicks / 2;
        }
        else
        {
            m_thunderTime--;
            if (m_thunderTime <= 0)
            {
                m_thundering = !m_thundering;
                OnWeatherChanged();
            }
        }

        if (m_rainTime <= 0)
        {
            m_rainTime = m_raining ? m_random.NextInt(0, WorldProvider::s_dayTicks / 2 - 1) + WorldProvider::s_dayTicks / 2
                                   : m_random.NextInt(0, WorldProvider::s_dayTicks * 7 - 1) + WorldProvider::s_dayTicks / 2;
        }
        else
        {
            m_rainTime--;
            if (m_rainTime <= 0)
            {
                m_raining = !m_raining;
                OnWeatherChanged();
            }
        }
    }

    Level::TickWeather();
    TickLightning();
    TickSnow();
    TickRandomBlocks();
}

void ServerLevel::AdoptBlockEntities(const ChunkPos &pos)
{
    Level::AdoptBlockEntities(pos);

    Chunk *chunk = GetChunk(pos);
    if (chunk->IsPendingMobSpawn())
    {
        chunk->SetPendingMobSpawn(false);
        if (GetDimensionId() != eDimensionId_Caves)
        {
            SpawnChunkGenerationMobs(chunk);
        }
    }
}

bool ServerLevel::CanMobSpawnAt(Entity *entity, const BlockPos &pos)
{
    if (dynamic_cast<Slime *>(entity))
    {
        int64_t chunkX = pos.x >> 4;
        int64_t chunkZ = pos.z >> 4;
        Random chunkRandom(
                (uint64_t) ((GetSeed() + chunkX * chunkX * 4987142 + chunkX * 5947611 + chunkZ * chunkZ * 4392871 + chunkZ * 389711) ^ 987234911));
        if ((GetGenSettings().worldType == eWorldType_Flat && m_random.NextInt(0, 3) != 0) || pos.y >= 40 || m_random.NextInt(0, 9) != 0 ||
            chunkRandom.NextInt(0, 9) != 0)
        {
            return false;
        }
    }
    else if (dynamic_cast<Monster *>(entity))
    {
        if (GetSkyLightLevel(pos) > m_random.NextInt(0, 31) || GetBlockLightLevel(pos) > 0)
        {
            return false;
        }

        int light = IsThundering() ? GetSkyLightLevel(pos) - 10 : GetRawBrightness(pos);
        if (light > m_random.NextInt(0, 7))
        {
            return false;
        }
    }
    else if (dynamic_cast<Squid *>(entity))
    {
        return pos.y > TerrainGenerator::s_seaLevel - 18 && pos.y < TerrainGenerator::s_seaLevel && !IntersectsBlock(entity->GetAABB());
    }
    else if (Block::ById(GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z))) != Block::ByName("grass_block") ||
             std::max((int) GetSkyLightLevel(pos), (int) GetBlockLightLevel(pos)) <= 8)
    {
        return false;
    }

    return !IntersectsBlock(entity->GetAABB()) && !ContainsLiquid(entity->GetAABB());
}

bool ServerLevel::PickSpawnEntry(int category, Biome *biome, SpawnEntry *entry)
{
    std::vector<SpawnEntry> entries;
    if (category == 0)
    {
        entries = {{eEntityType_Zombie, 95, 4, 4},
                   {eEntityType_Skeleton, 100, 4, 4},
                   {eEntityType_Spider, 100, 4, 4},
                   {eEntityType_Creeper, 100, 4, 4},
                   {eEntityType_Slime, 100, 4, 4}};
    }
    else if (category == 2)
    {
        entries = {{eEntityType_Squid, 1, 1, 4}};
    }
    else
    {
        if (biome->SpawnsAnimals())
        {
            entries.push_back({eEntityType_Sheep, 12, 4, 4});
            entries.push_back({eEntityType_Pig, 10, 4, 4});
            entries.push_back({eEntityType_Cow, 8, 4, 4});
        }

        if (biome->SpawnsChickens())
        {
            entries.push_back({eEntityType_Chicken, 10, 4, 4});
        }

        if (biome->SpawnsJungleMobs())
        {
            entries.push_back({eEntityType_Parrot, 40, 1, 2});
            entries.push_back({eEntityType_Ocelot, 2, 1, 3});
        }
    }

    int totalWeight = 0;
    for (size_t i = 0; i < entries.size(); i++)
    {
        totalWeight += entries[i].weight;
    }

    if (totalWeight == 0)
    {
        return false;
    }

    int roll = m_random.NextInt(0, totalWeight - 1);
    for (size_t i = 0; i < entries.size(); i++)
    {
        roll -= entries[i].weight;
        if (roll < 0)
        {
            *entry = entries[i];
            break;
        }
    }

    return true;
}

bool ServerLevel::SpawnMobAt(std::unique_ptr<Entity> entity, const BlockPos &pos)
{
    entity->SetRotation(m_random.NextFloat() * 360.0f, 0.0f);
    if (!CanMobSpawnAt(entity.get(), pos))
    {
        return false;
    }

    Mob *mob = (Mob *) entity.get();
    AddEntity(std::move(entity));
    mob->FinalizeSpawn();
    return true;
}

void ServerLevel::TickMobSpawning()
{
    constexpr int kMobCaps[]     = {70, 10, 5};
    constexpr double kSpawnRange = 128.0;

    std::vector<ServerPlayer *> players;
    std::unordered_set<ChunkPos, ChunkPosHash> chunks;
    const std::vector<std::unique_ptr<ServerPlayer>> &allPlayers = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < allPlayers.size(); i++)
    {
        if (allPlayers[i]->GetLevel() != this)
        {
            continue;
        }

        players.push_back(allPlayers[i].get());
        const glm::dvec3 &position = allPlayers[i]->GetPosition();
        int playerChunkX           = (int) floor(position.x / Chunk::s_sizeX);
        int playerChunkZ           = (int) floor(position.z / Chunk::s_sizeZ);
        for (int x = -8; x <= 8; x++)
        {
            for (int z = -8; z <= 8; z++)
            {
                ChunkPos pos(playerChunkX + x, playerChunkZ + z);
                double deltaX = pos.x * Chunk::s_sizeX + 8.0 - position.x;
                double deltaZ = pos.z * Chunk::s_sizeZ + 8.0 - position.z;
                if (deltaX * deltaX + deltaZ * deltaZ <= kSpawnRange * kSpawnRange && HasChunk(pos) &&
                    IsInSimulationDistance(glm::dvec3(pos.x * Chunk::s_sizeX + 8.0, 0.0, pos.z * Chunk::s_sizeZ + 8.0)))
                {
                    chunks.insert(pos);
                }
            }
        }
    }

    // Every mob counts toward the cap of each player near its chunk, so a crowded cave near one player doesn't stop spawns around another
    int counts[3] = {};
    std::vector<std::array<int, 3>> playerCounts(players.size(), {0, 0, 0});
    const std::vector<std::unique_ptr<Entity>> &entities = GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        int category   = dynamic_cast<Monster *>(entity) ? 0 : dynamic_cast<Squid *>(entity) ? 2 : dynamic_cast<Mob *>(entity) ? 1 : -1;
        // Mobs outside the simulation distance never tick, so they can't despawn and would hold the cap forever
        if (category < 0 || !IsInSimulationDistance(entity->GetPosition()))
        {
            continue;
        }

        counts[category]++;
        double centerX = floor(entity->GetPosition().x / Chunk::s_sizeX) * Chunk::s_sizeX + 8.0;
        double centerZ = floor(entity->GetPosition().z / Chunk::s_sizeZ) * Chunk::s_sizeZ + 8.0;
        for (size_t p = 0; p < players.size(); p++)
        {
            double deltaX = centerX - players[p]->GetPosition().x;
            double deltaZ = centerZ - players[p]->GetPosition().z;
            if (deltaX * deltaX + deltaZ * deltaZ <= kSpawnRange * kSpawnRange)
            {
                playerCounts[p][category]++;
            }
        }
    }

    const BlockPos &worldSpawn = GetWorldSpawn();
    bool creatureTick          = GetGameTime() % 400 == 0;
    for (const ChunkPos &chunkPos : chunks)
    {
        double centerX = chunkPos.x * Chunk::s_sizeX + 8.0;
        double centerZ = chunkPos.z * Chunk::s_sizeZ + 8.0;
        for (int category = 0; category < 3; category++)
        {
            if ((category == 0 && GetDifficulty() == 0) || (category == 1 && (!creatureTick || GetDimensionId() == eDimensionId_Caves)) ||
                counts[category] >= kMobCaps[category] * (int) chunks.size() / 289)
            {
                continue;
            }

            bool belowLocalCap = false;
            for (size_t p = 0; p < players.size() && !belowLocalCap; p++)
            {
                double deltaX = centerX - players[p]->GetPosition().x;
                double deltaZ = centerZ - players[p]->GetPosition().z;
                belowLocalCap = deltaX * deltaX + deltaZ * deltaZ <= kSpawnRange * kSpawnRange && playerCounts[p][category] < kMobCaps[category];
            }

            if (!belowLocalCap)
            {
                continue;
            }

            int startX   = chunkPos.x * Chunk::s_sizeX + m_random.NextInt(0, Chunk::s_sizeX - 1);
            int startZ   = chunkPos.z * Chunk::s_sizeZ + m_random.NextInt(0, Chunk::s_sizeZ - 1);
            int y        = m_random.NextInt(0, GetRainHeight(startX, startZ));
            Block *start = Block::ById(GetBlockId(BlockPos(startX, y, startZ)));
            if (category == 2 ? !start->IsLiquid() || start->IsLava() : start->IsSolid())
            {
                continue;
            }

            int spawned = 0;
            for (int group = 0; group < 3 && spawned < 4; group++)
            {
                int x            = startX;
                int z            = startZ;
                SpawnEntry entry = {};
                int groupSize    = 0;
                int groupSpawned = 0;
                int tries        = (int) ceil(m_random.NextFloat() * 4.0f);
                for (int step = 0; step < tries && spawned < 4 && (groupSize == 0 || groupSpawned < groupSize); step++)
                {
                    x += m_random.NextInt(0, 5) - m_random.NextInt(0, 5);
                    z += m_random.NextInt(0, 5) - m_random.NextInt(0, 5);
                    Chunk *chunk = GetChunk(ChunkPos(Mth::FloorDiv(x, Chunk::s_sizeX), Mth::FloorDiv(z, Chunk::s_sizeZ)));
                    if (!chunk)
                    {
                        continue;
                    }

                    glm::dvec3 position(x + 0.5, y, z + 0.5);
                    Player *nearest = FindNearestPlayer(position, kSpawnRange);
                    if (!nearest || glm::distance2(nearest->GetPosition(), position) <= 24.0 * 24.0 ||
                        glm::distance2(glm::dvec3(worldSpawn.x + 0.5, worldSpawn.y, worldSpawn.z + 0.5), position) < 24.0 * 24.0)
                    {
                        continue;
                    }

                    BlockPos pos(x, y, z);
                    Block *below = Block::ById(GetBlockId(BlockPos(x, y - 1, z)));
                    Block *block = Block::ById(GetBlockId(pos));
                    Block *above = Block::ById(GetBlockId(BlockPos(x, y + 1, z)));
                    if (category == 2)
                    {
                        if (!block->IsLiquid() || block->IsLava() || above->IsSolid())
                        {
                            continue;
                        }
                    }
                    else if (below == Block::ByName("bedrock") || !below->IsFaceSturdy(GetBlockData(BlockPos(x, y - 1, z)), Direction::up) ||
                             block->IsSolid() || block->IsLiquid() || above->IsSolid())
                    {
                        continue;
                    }

                    if (groupSize == 0)
                    {
                        if (!PickSpawnEntry(category, chunk->GetBiomeAt(x & (Chunk::s_sizeX - 1), y, z & (Chunk::s_sizeZ - 1)), &entry))
                        {
                            break;
                        }

                        groupSize = entry.minCount + m_random.NextInt(0, entry.maxCount - entry.minCount);
                    }

                    std::unique_ptr<Entity> entity = EntityFactory::Create(entry.type, this);
                    entity->SetPosition(position);
                    if (!SpawnMobAt(std::move(entity), pos))
                    {
                        continue;
                    }

                    spawned++;
                    groupSpawned++;
                    counts[category]++;
                    for (size_t p = 0; p < players.size(); p++)
                    {
                        double deltaX = centerX - players[p]->GetPosition().x;
                        double deltaZ = centerZ - players[p]->GetPosition().z;
                        if (deltaX * deltaX + deltaZ * deltaZ <= kSpawnRange * kSpawnRange)
                        {
                            playerCounts[p][category]++;
                        }
                    }
                }
            }
        }
    }
}

void ServerLevel::SpawnChunkGenerationMobs(Chunk *chunk)
{
    int minX     = chunk->GetPos().x * Chunk::s_sizeX;
    int minZ     = chunk->GetPos().z * Chunk::s_sizeZ;
    Biome *biome = chunk->GetBiomeAt(0, GetRainHeight(minX, minZ), 0);
    while (m_random.NextFloat() < 0.1f)
    {
        SpawnEntry entry = {};
        if (!PickSpawnEntry(1, biome, &entry))
        {
            return;
        }

        int count  = entry.minCount + m_random.NextInt(0, entry.maxCount - entry.minCount);
        int startX = minX + m_random.NextInt(0, Chunk::s_sizeX - 1);
        int startZ = minZ + m_random.NextInt(0, Chunk::s_sizeZ - 1);
        int x      = startX;
        int z      = startZ;
        for (int i = 0; i < count; i++)
        {
            bool placed = false;
            for (int attempt = 0; attempt < 4 && !placed; attempt++)
            {
                std::unique_ptr<Entity> entity = EntityFactory::Create(entry.type, this);
                double halfWidth               = ((Mob *) entity.get())->GetWidth() / 2.0;
                int y                          = GetRainHeight(x, z);
                entity->SetPosition(glm::dvec3(std::clamp(x + 0.5, minX + halfWidth, minX + Chunk::s_sizeX - halfWidth), y,
                                               std::clamp(z + 0.5, minZ + halfWidth, minZ + Chunk::s_sizeZ - halfWidth)));
                placed = SpawnMobAt(std::move(entity), BlockPos(x, y, z));

                x += m_random.NextInt(0, 4) - m_random.NextInt(0, 4);
                z += m_random.NextInt(0, 4) - m_random.NextInt(0, 4);
                while (x < minX || x >= minX + Chunk::s_sizeX || z < minZ || z >= minZ + Chunk::s_sizeZ)
                {
                    x = startX + m_random.NextInt(0, 4) - m_random.NextInt(0, 4);
                    z = startZ + m_random.NextInt(0, 4) - m_random.NextInt(0, 4);
                }
            }
        }
    }
}

void ServerLevel::TickLightning()
{
    const int strikeChance = 100000;

    if (!IsRaining() || !IsThundering())
    {
        return;
    }

    for (size_t i = 0; i < m_simulationChunks.size(); i++)
    {
        int chunkX = m_simulationChunks[i].x;
        int chunkZ = m_simulationChunks[i].z;
        if (m_random.NextInt(0, strikeChance - 1) != 0 || !HasChunk(ChunkPos(chunkX, chunkZ)))
        {
            continue;
        }

        m_lightningRandomValue = m_lightningRandomValue * 3 + 1013904223;
        uint32_t value         = m_lightningRandomValue >> 2;
        int x                  = chunkX * Chunk::s_sizeX + (int) (value & 15);
        int z                  = chunkZ * Chunk::s_sizeZ + (int) ((value >> 8) & 15);
        BlockPos pos(x, GetRainHeight(x, z), z);
        if (IsRainingAt(pos))
        {
            std::unique_ptr<Entity> lightning(new LightningBolt(this));
            lightning->SetPosition(glm::dvec3(x + 0.5, pos.y, z + 0.5));
            lightning->ReadSpawnData((int) m_random.NextUInt());
            AddEntity(std::move(lightning));
        }
    }
}

void ServerLevel::TickSnow()
{
    const int snowChance       = 16;
    const int freezeLightLevel = 10;

    bool raining         = IsRaining();
    SnowLayerBlock *snow = (SnowLayerBlock *) Block::ByName("snow");
    Block *ice           = Block::ByName("ice");
    Block *water         = Block::ByName("water_still");
    for (size_t i = 0; i < m_simulationChunks.size(); i++)
    {
        int chunkX = m_simulationChunks[i].x;
        int chunkZ = m_simulationChunks[i].z;
        if (m_random.NextInt(0, snowChance - 1) != 0)
        {
            continue;
        }

        const Chunk *chunk = GetChunk(ChunkPos(chunkX, chunkZ));
        if (!chunk)
        {
            continue;
        }

        int localX = m_random.NextInt(0, Chunk::s_sizeX - 1);
        int localZ = m_random.NextInt(0, Chunk::s_sizeZ - 1);
        int x      = chunkX * Chunk::s_sizeX + localX;
        int z      = chunkZ * Chunk::s_sizeZ + localZ;
        BlockPos pos(x, GetRainHeight(x, z), z);
        if (pos.y <= 0 || pos.y >= Chunk::s_sizeY || !chunk->GetSurfaceBiome(localX, localZ)->IsSnowyAt(pos.y))
        {
            continue;
        }

        BlockPos surface(x, pos.y - 1, z);
        if (Block::ById(GetBlockId(surface)) == water && GetBlockData(surface) == 0 && GetBlockLightLevel(surface) < freezeLightLevel)
        {
            SetBlock(surface, ice);
            RecordBlockChange(surface);
            continue;
        }

        if (!raining || !IsSnowingAt(pos))
        {
            continue;
        }

        Block *below = Block::ById(GetBlockId(surface));
        if (below == snow)
        {
            // We let snow pile a couple layers past its drift depth during storms, thicker piles grow slower
            int layers = snow->GetLayers(GetBlockData(surface));
            if (layers < std::min(snow->GetDriftLayers(GetSeed(), x, z) + 2, LayerBlock::kMaxLayers) && m_random.NextInt(0, layers - 1) == 0)
            {
                SetBlockData(surface, snow->WithLayers(layers + 1));
                RecordBlockChange(surface);
            }

            continue;
        }

        if (LeavesBlock::IsLeaves(below) && GetBlockId(pos) != 0)
        {
            while (surface.y > 0 && (LeavesBlock::IsLeaves(below) || GetBlockId(surface) == 0))
            {
                surface = BlockPos(x, surface.y - 1, z);
                below   = Block::ById(GetBlockId(surface));
            }

            pos = BlockPos(x, surface.y + 1, z);
        }

        if (pos.y <= 0 || GetBlockId(pos) != 0 || !below->IsSolid() || below->IsLiquid() || below->GetRenderShape() != Block::eRenderShape_CUBE)
        {
            continue;
        }

        SetBlock(pos, snow);
        RecordBlockChange(pos);
    }
}

void ServerLevel::TickRandomBlocks()
{
    const int ticksPerSection = GetGameRules().GetInt(eGameRule_RandomTickSpeed);
    if (ticksPerSection <= 0)
    {
        return;
    }

    for (size_t i = 0; i < m_simulationChunks.size(); i++)
    {
        int chunkX         = m_simulationChunks[i].x;
        int chunkZ         = m_simulationChunks[i].z;
        const Chunk *chunk = GetChunk(ChunkPos(chunkX, chunkZ));
        if (!chunk)
        {
            continue;
        }

        for (int sectionIndex = 0; sectionIndex < Chunk::s_sectionCount; sectionIndex++)
        {
            const ChunkSection &section = chunk->GetSection(sectionIndex);
            section.PickRandomTicks(&m_random, ticksPerSection, &m_randomTickScratch);
            int sectionY = sectionIndex * ChunkSection::s_size;
            for (size_t n = 0; n < m_randomTickScratch.size(); n++)
            {
                int index    = m_randomTickScratch[n];
                int x        = index & 15;
                int y        = (index >> 4) & 15;
                int z        = index >> 8;
                Block *block = Block::ById(section.GetBlockId(x, y, z));
                if (block->IsRandomTicking())
                {
                    block->RandomTick(this, BlockPos(chunkX * Chunk::s_sizeX + x, sectionY + y, chunkZ * Chunk::s_sizeZ + z));
                }
            }
        }
    }
}

void ServerLevel::ScheduleBlockForTick(const BlockPos &pos, uint32_t delayTicks, int priority)
{
    Block *block = Block::ById(GetBlockId(pos));
    std::unordered_map<BlockPos, ScheduledBlockTick> &chunkTicks =
            m_chunkBlockTicks[ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ))];
    auto scheduled = chunkTicks.find(pos);
    if (scheduled != chunkTicks.end() && scheduled->second.block == block)
    {
        return;
    }

    if (scheduled == chunkTicks.end())
    {
        m_scheduledBlockTickCount++;
    }

    ScheduledBlockTick tick{GetGameTime() + (int64_t) delayTicks, priority, m_nextTickSequence++, pos, block};
    chunkTicks[pos] = tick;
    m_scheduledBlockTicks.push(tick);
}

size_t ServerLevel::GetScheduledBlockTickCount() const
{
    return m_scheduledBlockTickCount;
}

void ServerLevel::StoreBlockTicks(Chunk *chunk, bool remove)
{
    const ChunkPos &chunkPos = chunk->GetPos();
    std::vector<ChunkBlockTick> blockTicks;
    auto chunkTicks = m_chunkBlockTicks.find(chunkPos);
    if (chunkTicks != m_chunkBlockTicks.end())
    {
        blockTicks.reserve(chunkTicks->second.size());
        for (auto tickIt = chunkTicks->second.begin(); tickIt != chunkTicks->second.end(); ++tickIt)
        {
            const ScheduledBlockTick &tick = tickIt->second;
            blockTicks.push_back(ChunkBlockTick{tick.block, tick.pos.x - chunkPos.x * Chunk::s_sizeX, tick.pos.y,
                                                tick.pos.z - chunkPos.z * Chunk::s_sizeZ, (int) std::max(tick.dueTick - GetGameTime(), (int64_t) 0),
                                                tick.priority});
        }

        if (remove)
        {
            m_scheduledBlockTickCount -= chunkTicks->second.size();
            m_chunkBlockTicks.erase(chunkTicks);
        }
    }

    if (remove)
    {
        m_parkedBlockTicks.erase(chunkPos);
    }

    chunk->SetBlockTicks(std::move(blockTicks));
}

void ServerLevel::ProcessScheduledBlockTicks(int64_t nowTick)
{
    const int maxPerTick = 65536;
    int processed        = 0;

    while (!m_scheduledBlockTicks.empty() && processed < maxPerTick)
    {
        const ScheduledBlockTick top = m_scheduledBlockTicks.top();
        if (top.dueTick > nowTick)
        {
            break;
        }

        m_scheduledBlockTicks.pop();
        // Forgotten and rescheduled ticks stay in the heap until they surface here
        if (!IsBlockTickScheduled(top))
        {
            continue;
        }

        processed++;

        if (!IsAreaLoaded(top.pos, 0))
        {
            ForgetScheduledBlockTick(top);
            continue;
        }

        if (!IsInSimulationDistance(glm::dvec3(top.pos.x, top.pos.y, top.pos.z)) || !IsAreaLoaded(top.pos, s_blockTickLoadedRadius))
        {
            m_parkedBlockTicks[ChunkPos(Mth::FloorDiv(top.pos.x, Chunk::s_sizeX), Mth::FloorDiv(top.pos.z, Chunk::s_sizeZ))].push_back(top);
            continue;
        }

        ForgetScheduledBlockTick(top);

        if (Block::ById(GetBlockId(top.pos)) != top.block)
        {
            continue;
        }

        top.block->Tick(this, top.pos);
    }
}

void ServerLevel::AddBlockEvent(const BlockPos &pos, Block *block, int id)
{
    BlockEvent event = {pos, block, id};
    if (std::find(m_blockEvents.begin(), m_blockEvents.end(), event) == m_blockEvents.end())
    {
        m_blockEvents.push_back(event);
    }
}

bool ServerLevel::IsHandlingBlockTicks() const
{
    return m_handlingBlockTicks;
}

void ServerLevel::RunBlockEvents()
{
    while (!m_blockEvents.empty())
    {
        BlockEvent event = m_blockEvents.front();
        m_blockEvents.pop_front();
        if (Block::ById(GetBlockId(event.pos)) == event.block)
        {
            event.block->TriggerEvent(this, event.pos, event.id);
        }
    }
}

bool ServerLevel::IsBlockTickScheduled(const ScheduledBlockTick &tick) const
{
    auto chunkTicks = m_chunkBlockTicks.find(ChunkPos(Mth::FloorDiv(tick.pos.x, Chunk::s_sizeX), Mth::FloorDiv(tick.pos.z, Chunk::s_sizeZ)));
    if (chunkTicks == m_chunkBlockTicks.end())
    {
        return false;
    }

    auto scheduled = chunkTicks->second.find(tick.pos);
    return scheduled != chunkTicks->second.end() && scheduled->second.sequence == tick.sequence;
}

void ServerLevel::ForgetScheduledBlockTick(const ScheduledBlockTick &tick)
{
    auto chunkTicks = m_chunkBlockTicks.find(ChunkPos(Mth::FloorDiv(tick.pos.x, Chunk::s_sizeX), Mth::FloorDiv(tick.pos.z, Chunk::s_sizeZ)));
    if (chunkTicks == m_chunkBlockTicks.end())
    {
        return;
    }

    auto scheduled = chunkTicks->second.find(tick.pos);
    if (scheduled == chunkTicks->second.end() || scheduled->second.sequence != tick.sequence)
    {
        return;
    }

    chunkTicks->second.erase(scheduled);
    m_scheduledBlockTickCount--;
    if (chunkTicks->second.empty())
    {
        m_chunkBlockTicks.erase(chunkTicks);
    }
}

void ServerLevel::UnparkBlockTicks()
{
    for (auto parkedIt = m_parkedBlockTicks.begin(); parkedIt != m_parkedBlockTicks.end();)
    {
        const ChunkPos &chunkPos = parkedIt->first;
        BlockPos center(chunkPos.x * Chunk::s_sizeX + Chunk::s_sizeX / 2, 0, chunkPos.z * Chunk::s_sizeZ + Chunk::s_sizeZ / 2);
        if (!IsAreaLoaded(center, 0))
        {
            for (size_t i = 0; i < parkedIt->second.size(); i++)
            {
                ForgetScheduledBlockTick(parkedIt->second[i]);
            }

            parkedIt = m_parkedBlockTicks.erase(parkedIt);
            continue;
        }

        if (!IsInSimulationDistance(glm::dvec3(center.x, 0, center.z)) || !IsAreaLoaded(center, Chunk::s_sizeX / 2 + s_blockTickLoadedRadius))
        {
            ++parkedIt;
            continue;
        }

        for (size_t i = 0; i < parkedIt->second.size(); i++)
        {
            ScheduledBlockTick tick = parkedIt->second[i];
            if (!IsBlockTickScheduled(tick))
            {
                continue;
            }

            tick.dueTick                                  = GetGameTime() + (int64_t) (i & 15);
            m_chunkBlockTicks[chunkPos][tick.pos].dueTick = tick.dueTick;
            m_scheduledBlockTicks.push(tick);
        }

        parkedIt = m_parkedBlockTicks.erase(parkedIt);
    }
}

void ServerLevel::UpdateSimulationCenters()
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    m_simulationCenterScratch.clear();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i]->GetLevel() == this)
        {
            const glm::dvec3 &position = players[i]->GetPosition();
            m_simulationCenterScratch.push_back(ChunkPos((int) floor(position.x) >> 4, (int) floor(position.z) >> 4));
        }
    }

    SetPlayerSimulationCenters(m_simulationCenterScratch);
}

void ServerLevel::PlaySound(const ResourceLocation &location, const glm::dvec3 &position, float volume, float pitch)
{
    glm::vec3 soundPosition((float) position.x, (float) position.y, (float) position.z);
    SendToNearby(position, volume > 1.0f ? 16.0 * volume : 16.0,
                 std::shared_ptr<Packet>(new SoundS2CPacket(location.ToString(), volume, pitch, soundPosition)));
}

void ServerLevel::RecordBlockChange(const BlockPos &pos)
{
    if (m_blockChangeSet.insert(pos).second)
    {
        m_blockChanges.push_back(pos);
    }
}

void ServerLevel::OnWeatherChanged()
{
    m_server->GetPlayerList()->SendToLevel(
            this, std::shared_ptr<Packet>(new SetWeatherS2CPacket(IsRaining(), IsThundering(), GetRainLevel(1.0f), GetThunderLevel(1.0f))));
}

void ServerLevel::OnEntityHurt(Entity *entity)
{
    m_server->GetPlayerList()->SendToLevel(this, std::shared_ptr<Packet>(new AnimateHurtS2CPacket(entity->GetId())));
}

void ServerLevel::OnMobDataChanged(Mob *mob)
{
    MobData data;
    mob->GetMobData(&data);
    m_server->GetPlayerList()->SendToLevel(this, std::shared_ptr<Packet>(new SetMobDataS2CPacket(mob->GetId(), data)));
}

void ServerLevel::BroadcastEntityEvent(Entity *entity, uint8_t event)
{
    m_server->GetPlayerList()->SendToLevel(this, std::shared_ptr<Packet>(new EntityEventS2CPacket(entity->GetId(), event)));
}

void ServerLevel::OnPassengersChanged(Entity *vehicle)
{
    const std::vector<Entity *> &passengers = vehicle->GetPassengers();
    std::vector<int> passengerIds(passengers.size());
    for (size_t i = 0; i < passengers.size(); i++)
    {
        passengerIds[i] = passengers[i]->GetId();
    }

    m_server->GetPlayerList()->SendToLevel(this, std::shared_ptr<Packet>(new SetPassengersS2CPacket(vehicle->GetId(), passengerIds)));
}

void ServerLevel::OnBoatDataChanged(Boat *boat)
{
    m_server->GetPlayerList()->SendToLevel(
            this, std::shared_ptr<Packet>(new SetBoatDataS2CPacket(boat->GetId(), boat->GetPaddleState(0), boat->GetPaddleState(1))));
}

void ServerLevel::AddEntity(std::unique_ptr<Entity> entity)
{
    if (entity->GetId() == 0)
    {
        entity->SetId(m_server->NextEntityId());
    }

    SendEntity(entity.get(), nullptr);
    if (dynamic_cast<Mob *>(entity.get()) || dynamic_cast<Boat *>(entity.get()) || dynamic_cast<Minecart *>(entity.get()))
    {
        m_server->GetPlayerList()->GetEntityTracker()->Track(entity.get());
    }

    Level::AddEntity(std::move(entity));
}

Entity *ServerLevel::GetEntityById(int id) const
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i]->GetId() == id && players[i]->GetLevel() == this)
        {
            return players[i].get();
        }
    }

    return Level::GetEntityById(id);
}

Player *ServerLevel::FindNearestPlayer(const glm::dvec3 &position, double maxDistance) const
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    Player *nearest                                           = nullptr;
    double nearestDistSq                                      = maxDistance * maxDistance;
    for (size_t i = 0; i < players.size(); i++)
    {
        ServerPlayer *player = players[i].get();
        if (player->GetLevel() != this || GameType::IsSpectator(player->GetGameType()) || player->GetHealth() <= 0)
        {
            continue;
        }

        double distSq = glm::distance2(player->GetPosition(), position);
        if (distSq < nearestDistSq)
        {
            nearest       = player;
            nearestDistSq = distSq;
        }
    }

    return nearest;
}

void ServerLevel::GetPushableEntities(const AABB &box, const Entity *except, std::vector<Entity *> *entities) const
{
    Level::GetPushableEntities(box, except, entities);
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        ServerPlayer *player = players[i].get();
        if (player != except && player->GetLevel() == this && player->GetHealth() > 0 && !GameType::IsSpectator(player->GetGameType()) &&
            player->GetAABB().Intersects(box))
        {
            entities->push_back(player);
        }
    }
}

void ServerLevel::RemoveEntity(Entity *entity)
{
    m_removedEntityIds.push_back(entity->GetId());
    if (dynamic_cast<Mob *>(entity) || dynamic_cast<Boat *>(entity) || dynamic_cast<Minecart *>(entity))
    {
        m_server->GetPlayerList()->GetEntityTracker()->Untrack(entity);
    }

    Level::RemoveEntity(entity);
}

EversteadServer *ServerLevel::GetServer() const
{
    return m_server;
}

void ServerLevel::SendEntitiesTo(ServerPlayer *player)
{
    PlayerConnection *connection = player->GetPlayerConnection();
    if (!connection)
    {
        return;
    }

    const std::vector<std::unique_ptr<Entity>> &entities = GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (!dynamic_cast<ServerPlayer *>(entities[i].get()))
        {
            SendEntity(entities[i].get(), connection);
        }
    }
}

void ServerLevel::SendEntity(Entity *entity, PlayerConnection *connection)
{
    std::shared_ptr<Packet> addPacket(new AddEntityS2CPacket(entity->GetId(), entity->GetType(), entity->GetPosition(), entity->GetVelocity(),
                                                             entity->GetYaw(), entity->GetPitch(), entity->GetSpawnData()));
    std::shared_ptr<Packet> dataPacket;
    ItemEntity *itemEntity = dynamic_cast<ItemEntity *>(entity);
    if (itemEntity)
    {
        dataPacket.reset(new SetEntityDataS2CPacket(itemEntity->GetId(), itemEntity->GetItem(), itemEntity->GetAge(), itemEntity->GetPickupDelay()));
    }

    Mob *mob = dynamic_cast<Mob *>(entity);
    if (mob)
    {
        MobData data;
        mob->GetMobData(&data);
        dataPacket.reset(new SetMobDataS2CPacket(mob->GetId(), data));
    }

    Boat *boat = dynamic_cast<Boat *>(entity);
    if (boat)
    {
        dataPacket.reset(new SetBoatDataS2CPacket(boat->GetId(), boat->GetPaddleState(0), boat->GetPaddleState(1)));
    }

    // Riders can be sent before or after their vehicle, so whichever arrives second links them
    std::shared_ptr<Packet> passengersPacket;
    Entity *vehicle = entity->GetVehicle() ? entity->GetVehicle() : entity;
    if (boat || !vehicle->GetPassengers().empty())
    {
        const std::vector<Entity *> &passengers = vehicle->GetPassengers();
        std::vector<int> passengerIds(passengers.size());
        for (size_t i = 0; i < passengers.size(); i++)
        {
            passengerIds[i] = passengers[i]->GetId();
        }

        passengersPacket.reset(new SetPassengersS2CPacket(vehicle->GetId(), passengerIds));
    }

    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        PlayerConnection *playerConnection = players[i]->GetPlayerConnection();
        if (players[i]->GetLevel() != this || !playerConnection || (connection && playerConnection != connection))
        {
            continue;
        }

        playerConnection->Send(addPacket);
        if (dataPacket)
        {
            playerConnection->Send(dataPacket);
        }

        if (passengersPacket)
        {
            playerConnection->Send(passengersPacket);
        }
    }
}

void ServerLevel::BroadcastRemovedEntities()
{
    if (m_removedEntityIds.empty())
    {
        return;
    }

    std::shared_ptr<Packet> packet(new RemoveEntitiesS2CPacket(m_removedEntityIds));
    m_removedEntityIds.clear();
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        PlayerConnection *connection = players[i]->GetPlayerConnection();
        if (players[i]->GetLevel() == this && connection)
        {
            connection->Send(packet);
        }
    }
}

void ServerLevel::SendToNearby(const glm::dvec3 &position, double range, const std::shared_ptr<Packet> &packet)
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        ServerPlayer *player         = players[i].get();
        PlayerConnection *connection = player->GetPlayerConnection();
        if (player->GetLevel() == this && connection && glm::distance2(player->GetPosition(), position) < range * range)
        {
            connection->Send(packet);
        }
    }
}

void ServerLevel::BroadcastBlockChanges()
{
    if (m_blockChanges.empty())
    {
        return;
    }

    std::vector<MultiBlockUpdateS2CPacket::Change> changes;
    changes.reserve(m_blockChanges.size());
    for (size_t i = 0; i < m_blockChanges.size(); i++)
    {
        const BlockPos &pos = m_blockChanges[i];
        changes.push_back({pos, (uint16_t) GetBlockId(pos), GetBlockData(pos)});
    }

    m_blockChanges.clear();
    m_blockChangeSet.clear();

    // We only send each player changes inside chunks it has loaded since the client drops the rest anyway
    std::vector<MultiBlockUpdateS2CPacket::Change> playerChanges;
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t p = 0; p < players.size(); p++)
    {
        ServerPlayer *player         = players[p].get();
        PlayerConnection *connection = player->GetPlayerConnection();
        if (player->GetLevel() != this || !connection)
        {
            continue;
        }

        playerChanges.clear();
        for (size_t i = 0; i < changes.size(); i++)
        {
            const BlockPos &pos = changes[i].pos;
            if (connection->GetChunkSender()->HasSentChunk(ChunkPos(pos.x >> 4, pos.z >> 4)))
            {
                playerChanges.push_back(changes[i]);
            }
        }

        if (!playerChanges.empty())
        {
            connection->Send(std::shared_ptr<Packet>(new MultiBlockUpdateS2CPacket(playerChanges)));
        }
    }
}

// Items are simulated here and on every client, the host resends one only when something the clients cannot predict changes
void ServerLevel::TickItems()
{
    m_itemScratch.clear();
    m_removedItemScratch.clear();

    const std::vector<std::unique_ptr<Entity>> &entities = GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        const glm::dvec3 &position = entities[i]->GetPosition();
        if (entities[i]->GetType() == eEntityType_Item && IsInSimulationDistance(position) &&
            IsAreaLoaded(BlockPos((int) floor(position.x), 0, (int) floor(position.z)), 1))
        {
            m_itemScratch.push_back((ItemEntity *) entities[i].get());
        }
    }

    // Items do not move during this pass, so merge candidates can be found by a range search on x
    std::sort(m_itemScratch.begin(), m_itemScratch.end(),
              [](const ItemEntity *a, const ItemEntity *b)
              {
                  return a->GetPosition().x < b->GetPosition().x;
              });

    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    Block *fire                                               = Block::ByName("fire");
    Block *cactus                                             = Block::ByName("cactus");
    constexpr int kLiquidResyncInterval                       = 20;
    for (size_t i = 0; i < m_itemScratch.size(); i++)
    {
        ItemEntity *itemEntity = m_itemScratch[i];
        if (itemEntity->GetItem().IsEmpty())
        {
            continue;
        }

        if (itemEntity->GetAge() >= ItemEntity::s_lifetime || itemEntity->GetPosition().y < s_itemVoidY)
        {
            itemEntity->SetItem(ItemInstance());
            continue;
        }

        if (itemEntity->IsInLava() || itemEntity->IsTouchingBlock(fire))
        {
            PlaySound("random.fizz", itemEntity->GetPosition(), 0.4f, 2.0f + m_random.NextFloat() * 0.4f);
            itemEntity->SetItem(ItemInstance());
            continue;
        }

        if (itemEntity->IsTouchingBlock(cactus))
        {
            itemEntity->SetItem(ItemInstance());
            continue;
        }

        // Floating items never land, so we correct their drift from water flow that clients simulate slightly differently
        bool changed =
                itemEntity->TakeLanded() || (itemEntity->IsInWater() && (itemEntity->GetAge() + itemEntity->GetId()) % kLiquidResyncInterval == 0);
        if (itemEntity->GetPickupDelay() == 0)
        {
            for (size_t p = 0; p < players.size() && !itemEntity->GetItem().IsEmpty(); p++)
            {
                if (players[p]->GetLevel() == this && TryPickup(players[p].get(), itemEntity))
                {
                    changed = true;
                }
            }
        }

        if (!itemEntity->GetItem().IsEmpty() && (itemEntity->GetAge() + itemEntity->GetId()) % s_mergeCheckInterval == 0 && TryMerge(itemEntity))
        {
            changed = true;
        }

        if (changed && !itemEntity->GetItem().IsEmpty())
        {
            SendEntity(itemEntity, nullptr);
        }
    }

    for (size_t i = 0; i < m_itemScratch.size(); i++)
    {
        if (m_itemScratch[i]->GetItem().IsEmpty())
        {
            m_removedItemScratch.push_back(m_itemScratch[i]);
            m_removedEntityIds.push_back(m_itemScratch[i]->GetId());
        }
    }

    RemoveEntities(&m_removedItemScratch);
}

bool ServerLevel::TryPickup(ServerPlayer *player, ItemEntity *itemEntity)
{
    if (GameType::IsSpectator(player->GetGameType()) || player->GetHealth() <= 0)
    {
        return false;
    }

    const AABB &playerBox = player->GetAABB();
    AABB reach(glm::dvec3(playerBox.GetMin().x - s_pickupReachXZ, playerBox.GetMin().y - s_pickupReachY, playerBox.GetMin().z - s_pickupReachXZ),
               glm::dvec3(playerBox.GetMax().x + s_pickupReachXZ, playerBox.GetMax().y + s_pickupReachY, playerBox.GetMax().z + s_pickupReachXZ));
    if (!reach.Intersects(itemEntity->GetAABB()))
    {
        return false;
    }

    Inventory *inventory = player->GetInventory();
    ItemInstance item    = itemEntity->GetItem();
    int before           = item.GetCount();
    inventory->AddItem(item);
    if (GameType::HasInfiniteItems(player->GetGameType()))
    {
        item = ItemInstance();
    }

    if (item.GetCount() == before && !item.IsEmpty())
    {
        return false;
    }

    PlayerList *playerList = m_server->GetPlayerList();
    playerList->SendToLevel(
            this, std::shared_ptr<Packet>(new TakeItemEntityS2CPacket(itemEntity->GetId(), player->GetPlayerIndex(), before - item.GetCount())));
    PlaySound("random.pop", player->GetPosition(), 0.2f, ((m_random.NextFloat() - m_random.NextFloat()) * 0.7f + 1.0f) * 2.0f);
    itemEntity->SetItem(item.GetCount() > 0 ? item : ItemInstance());
    playerList->SendInventoryTo(player);

    return true;
}

void ServerLevel::TickExperienceOrbs()
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    const std::vector<std::unique_ptr<Entity>> &entities      = GetEntities();
    m_removedItemScratch.clear();
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i]->GetType() != eEntityType_ExperienceOrb)
        {
            continue;
        }

        ExperienceOrb *orb = (ExperienceOrb *) entities[i].get();
        bool taken         = false;
        for (size_t p = 0; p < players.size() && !taken; p++)
        {
            ServerPlayer *player = players[p].get();
            if (player->GetLevel() != this || GameType::IsSpectator(player->GetGameType()) || player->GetHealth() <= 0 ||
                player->GetExperienceCooldown() > 0)
            {
                continue;
            }

            const AABB &playerBox = player->GetAABB();
            AABB reach(
                    glm::dvec3(playerBox.GetMin().x - s_pickupReachXZ, playerBox.GetMin().y - s_pickupReachY, playerBox.GetMin().z - s_pickupReachXZ),
                    glm::dvec3(playerBox.GetMax().x + s_pickupReachXZ, playerBox.GetMax().y + s_pickupReachY,
                               playerBox.GetMax().z + s_pickupReachXZ));
            if (!reach.Intersects(orb->GetAABB()))
            {
                continue;
            }

            player->SetExperienceCooldown(2);
            PlaySound("random.orb", orb->GetPosition(), 0.1f, 0.5f * ((m_random.NextFloat() - m_random.NextFloat()) * 0.7f + 1.8f));
            player->PickupExperience(orb->GetValue());
            m_removedItemScratch.push_back(orb);
            m_removedEntityIds.push_back(orb->GetId());
            taken = true;
        }

        if (!taken && orb->TakeTargetChanged())
        {
            SendEntity(orb, nullptr);
        }
    }

    RemoveEntities(&m_removedItemScratch);
}

bool ServerLevel::TryMerge(ItemEntity *itemEntity)
{
    ItemInstance &item = itemEntity->GetItem();
    if (!item.IsStackable() || item.GetCount() >= item.GetMaxStackSize())
    {
        return false;
    }

    const glm::dvec3 &position = itemEntity->GetPosition();
    size_t start               = std::lower_bound(m_itemScratch.begin(), m_itemScratch.end(), position.x - s_mergeRadius,
                                                  [](const ItemEntity *other, double x)
                                                  {
                                        return other->GetPosition().x < x;
                                                  }) -
                                 m_itemScratch.begin();
    bool merged                = false;
    for (size_t i = start; i < m_itemScratch.size() && m_itemScratch[i]->GetPosition().x <= position.x + s_mergeRadius; i++)
    {
        ItemEntity *other = m_itemScratch[i];
        if (other == itemEntity || other->GetItem().IsEmpty() || !other->GetItem().IsSameItem(item) ||
            other->GetItem().GetCount() > item.GetCount() || item.GetCount() + other->GetItem().GetCount() > item.GetMaxStackSize())
        {
            continue;
        }

        if (glm::distance2(other->GetPosition(), position) > s_mergeRadius * s_mergeRadius)
        {
            continue;
        }

        item.SetCount(item.GetCount() + other->GetItem().GetCount());
        itemEntity->SetPickupDelay(std::max(itemEntity->GetPickupDelay(), other->GetPickupDelay()));
        other->SetItem(ItemInstance());
        merged = true;
    }

    return merged;
}

void ServerLevel::DropContainerItems(const BlockPos &pos, AbstractContainer *container)
{
    for (int i = 0; i < container->GetSize(); i++)
    {
        ItemInstance item = *container->GetItem(i);
        if (item.IsEmpty())
        {
            continue;
        }

        glm::dvec3 position(pos.x + m_random.NextFloat() * 0.8 + 0.1, pos.y + m_random.NextFloat() * 0.8 + 0.1,
                            pos.z + m_random.NextFloat() * 0.8 + 0.1);
        while (item.GetCount() > 0)
        {
            int count = std::min(m_random.NextInt(10, 30), item.GetCount());
            item.SetCount(item.GetCount() - count);

            ItemInstance dropped = item;
            dropped.SetCount(count);
            glm::dvec3 velocity(m_random.NextGaussian() * 0.05, m_random.NextGaussian() * 0.05 + 0.2, m_random.NextGaussian() * 0.05);
            std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(this));
            itemEntity->Sync(position, velocity, dropped, 0, 10);
            AddEntity(std::move(itemEntity));
        }
    }
}

void ServerLevel::MarkBlockChunkDirty(const BlockPos &pos)
{
    GetChunk(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)))->MarkDirty();
    if (!Block::ById(GetBlockId(pos))->HasAnalogOutput())
    {
        return;
    }

    Direction *const directions[] = {Direction::north, Direction::south, Direction::west, Direction::east};
    Block *comparator             = Block::ByName("comparator");
    for (int i = 0; i < 4; i++)
    {
        BlockPos neighbor(pos.x + directions[i]->dx, pos.y, pos.z + directions[i]->dz);
        Block *block = Block::ById(GetBlockId(neighbor));
        if (block->IsSignalConductor())
        {
            neighbor = BlockPos(neighbor.x + directions[i]->dx, pos.y, neighbor.z + directions[i]->dz);
            block    = Block::ById(GetBlockId(neighbor));
        }

        if (block == comparator)
        {
            block->OnNeighborChanged(this, neighbor);
        }
    }
}
