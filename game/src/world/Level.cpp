#include "Level.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <mutex>
#include <numbers>
#include <shared_mutex>
#include <unordered_set>

#include "../core/FrameProfiler.h"
#include "../core/Logger.h"
#include "../core/TickProfiler.h"
#include "../core/TickRateManager.h"
#include "../entity/Entity.h"
#include "../entity/player/Player.h"
#include "../tag/CompoundTag.h"
#include "../utils/Direction.h"
#include "../utils/Random.h"
#include "../utils/StringUtils.h"
#include "../utils/Time.h"
#include "../utils/UniqueID.h"
#include "../utils/math/Mth.h"
#include "block/Block.h"
#include "block/GravityBlock.h"
#include "block/IceBlock.h"
#include "block/LeavesBlock.h"
#include "block/PointedDripstoneBlock.h"
#include "chunk/ChunkManager.h"
#include "generation/TerrainGenerator.h"
#include "item/Item.h"
#include "lighting/LightEngine.h"
#include "particle/ParticleSpawnParams.h"

#include <glm/gtx/norm.hpp>

constexpr int kChunkCacheSize = 64;

struct ChunkLookupCache
{
    uint64_t levelId;
    uint64_t epoch;
    ChunkPos positions[kChunkCacheSize];
    Chunk *chunks[kChunkCacheSize];
};

static std::atomic<uint64_t> s_nextLevelId{1};
static thread_local ChunkLookupCache s_chunkCache = {};

static size_t FindNearest(const std::deque<ChunkPos> &queue, const ChunkPos &center)
{
    size_t nearest   = 0;
    int64_t nearest2 = INT64_MAX;
    for (size_t i = 0; i < queue.size(); i++)
    {
        int64_t dx = (int64_t) queue[i].x - center.x;
        int64_t dz = (int64_t) queue[i].z - center.z;
        if (dx * dx + dz * dz < nearest2)
        {
            nearest  = i;
            nearest2 = dx * dx + dz * dz;
        }
    }

    return nearest;
}

const int Level::s_maxSpawnAreaRadius;
const float Level::s_breakSoundPitch  = 0.8f;
const float Level::s_placeSoundPitch  = 0.8f;
const float Level::s_placeSoundVolume = 1.0f;

Level::Level()
{
    m_updatingNeighbors          = false;
    m_nextBlockEntityTickOrder   = 0;
    m_chunkManager               = nullptr;
    m_dimensionId                = eDimensionId_Overworld;
    m_gameRules                  = &m_ownGameRules;
    m_scoreboard                 = &m_ownScoreboard;
    m_tickingEntities            = false;
    m_difficulty                 = 2;
    m_defaultGameType            = eGameType_Survival;
    m_onlineGame                 = false;
    m_allowFriendsOfFriends      = false;
    m_maxPlayers                 = 8;
    m_spawnAreaRadius            = 10;
    m_simulationDistance         = 10;
    m_hasWorldSpawn              = false;
    m_seed                       = 0;
    m_gameTime                   = 0;
    m_raining                    = false;
    m_thundering                 = false;
    m_rainTime                   = 0;
    m_thunderTime                = 0;
    m_rainLevel                  = 0.0f;
    m_oldRainLevel               = 0.0f;
    m_thunderLevel               = 0.0f;
    m_oldThunderLevel            = 0.0f;
    m_lightningFlashAge          = -1;
    m_lightningFlashStrength     = 0.0f;
    m_lightningFlashPosition     = glm::dvec3();
    m_emptyChunksSolid           = false;
    m_emptyChunksSolidForMeshing = false;
    m_renderDistance             = 10;
    m_id                         = s_nextLevelId.fetch_add(1);
    m_removeEpoch                = 0;
}

Level::~Level()
{}

void Level::SetChunkManager(ChunkManager *chunkManager)
{
    m_chunkManager = chunkManager;
}

ChunkManager *Level::GetChunkManager() const
{
    return m_chunkManager;
}

void Level::SetDimensionId(eDIMENSION_ID dimensionId)
{
    m_dimensionId = dimensionId;
}

eDIMENSION_ID Level::GetDimensionId() const
{
    return m_dimensionId;
}

void Level::Tick()
{
    m_simulationCenters = m_playerSimulationCenters;

    // Players close together share chunks, each chunk is listed once so weather and random ticks don't scale with player count
    std::unordered_set<ChunkPos, ChunkPosHash> seenChunks;
    m_simulationChunks.clear();
    for (size_t i = 0; i < m_simulationCenters.size(); i++)
    {
        const ChunkPos &center = m_simulationCenters[i];
        for (int chunkX = center.x - m_simulationDistance; chunkX <= center.x + m_simulationDistance; chunkX++)
        {
            for (int chunkZ = center.z - m_simulationDistance; chunkZ <= center.z + m_simulationDistance; chunkZ++)
            {
                if (seenChunks.insert(ChunkPos(chunkX, chunkZ)).second)
                {
                    m_simulationChunks.push_back(ChunkPos(chunkX, chunkZ));
                }
            }
        }
    }

    {
        FrameProfilerScope scope("Level.Entities");
        TickEntities();
    }

    if (!GetTickRateManager()->RunsNormally())
    {
        return;
    }

    {
        FrameProfilerScope scope("Level.BlockEntities");
        TickBlockEntities();
    }

    {
        FrameProfilerScope scope("Level.Weather");
        TickWeather();
    }

    m_worldBorder.Tick();
}

// Game time advances before block ticks and block events because pistons compare it against when their moving blocks last ticked
void Level::TickTime()
{
    m_gameTime++;
    if (m_gameRules->GetBool(eGameRule_DoDaylightCycle))
    {
        m_worldProvider.TickWorldTime();
    }
}

void Level::TickWeather()
{
    const float levelStep = 0.01f;

    m_oldRainLevel    = m_rainLevel;
    m_oldThunderLevel = m_thunderLevel;
    m_rainLevel       = std::clamp(m_rainLevel + (m_raining ? levelStep : -levelStep), 0.0f, 1.0f);
    m_thunderLevel    = std::clamp(m_thunderLevel + (m_thundering ? levelStep : -levelStep), 0.0f, 1.0f);

    if (m_lightningFlashAge >= 0)
    {
        m_lightningFlashAge++;
    }
}

void Level::UpdateChunks(int chunkIntakeBudget)
{
    if (!m_chunkManager)
    {
        return;
    }

    std::deque<std::pair<ChunkPos, std::unique_ptr<Chunk>>> ready;
    m_chunkManager->DrainFinished(&ready, chunkIntakeBudget);
    for (int intook = 0; !ready.empty() && intook < chunkIntakeBudget; intook++)
    {
        ChunkPos pos = ready.front().first;
        if (!AdoptChunk(pos, std::move(ready.front().second)))
        {
            ready.pop_front();
            continue;
        }

        ready.pop_front();
        BlockPos blockPos(pos.x * Chunk::s_sizeX, 0, pos.z * Chunk::s_sizeZ);
        MarkChunkDirty(blockPos);
        if (HasChunk(ChunkPos(pos.x - 1, pos.z)))
        {
            MarkChunkDirty(BlockPos(blockPos.x - Chunk::s_sizeX, blockPos.y, blockPos.z));
        }

        if (HasChunk(ChunkPos(pos.x + 1, pos.z)))
        {
            MarkChunkDirty(BlockPos(blockPos.x + Chunk::s_sizeX, blockPos.y, blockPos.z));
        }

        if (HasChunk(ChunkPos(pos.x, pos.z - 1)))
        {
            MarkChunkDirty(BlockPos(blockPos.x, blockPos.y, blockPos.z - Chunk::s_sizeZ));
        }

        if (HasChunk(ChunkPos(pos.x, pos.z + 1)))
        {
            MarkChunkDirty(BlockPos(blockPos.x, blockPos.y, blockPos.z + Chunk::s_sizeZ));
        }
    }
}

bool Level::AdoptChunk(const ChunkPos &pos, std::unique_ptr<Chunk> chunk)
{
    if (!InsertChunk(pos, std::move(chunk)))
    {
        return false;
    }

    ScheduleWaterTicks(pos);
    ScheduleSavedBlockTicks(pos);
    AdoptBlockEntities(pos);
    QueueChunkSkyLightSeamUpdate(pos);
    return true;
}

void Level::UpdateLighting(int budgetMicroseconds)
{
    const int lightBudget = 32;
    const std::chrono::microseconds lightTimeBudget(budgetMicroseconds);
    std::chrono::steady_clock::time_point lightStart = std::chrono::steady_clock::now();
    for (int processed = 0; processed < lightBudget; processed++)
    {
        if (processed > 0 && std::chrono::steady_clock::now() - lightStart > lightTimeBudget)
        {
            break;
        }

        BlockPos pos;
        if (!PollLightUpdate(&pos))
        {
            break;
        }

        LightEngine::UpdateFrom(this, pos);
    }

    const int skyLightSeamBudget = 2;
    ChunkPos center              = GetPriorityChunk();
    for (int processed = 0; processed < skyLightSeamBudget; processed++)
    {
        if (processed > 0 && std::chrono::steady_clock::now() - lightStart > lightTimeBudget)
        {
            break;
        }

        ChunkPos pos;
        if (!PollSkyLightSeamUpdate(&pos, center))
        {
            break;
        }

        LightEngine::UpdateChunkSkyLightSeams(this, pos);
        LightEngine::PropagateBlockLight(this, pos);
    }
}

ChunkPos Level::GetPriorityChunk() const
{
    if (m_simulationCenters.empty())
    {
        return ChunkPos(0, 0);
    }

    return m_simulationCenters[0];
}

Chunk *Level::GetChunk(const ChunkPos &pos)
{
    return FindChunk(pos);
}

const Chunk *Level::GetChunk(const ChunkPos &pos) const
{
    return FindChunk(pos);
}

Chunk &Level::CreateChunk(const ChunkPos &pos)
{
    std::unique_lock<std::shared_mutex> lock(m_chunksMutex);

    auto it = m_chunks.find(pos);
    if (it != m_chunks.end())
    {
        return *it->second;
    }

    std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(pos);
    Chunk &ref                   = *chunk;

    m_chunks.emplace(pos, std::move(chunk));

    Logger::LogInfo("Created chunk (%d, %d)", pos.x, pos.z);

    return ref;
}

bool Level::HasChunk(const ChunkPos &pos) const
{
    return FindChunk(pos) != nullptr;
}

std::vector<std::pair<ChunkPos, Chunk *>> Level::GetLoadedChunks() const
{
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);

    std::vector<std::pair<ChunkPos, Chunk *>> loadedChunks;
    loadedChunks.reserve(m_chunks.size());

    for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it)
    {
        loadedChunks.push_back(std::make_pair(it->first, it->second.get()));
    }

    return loadedChunks;
}

void Level::CollectLoadedChunkPositions(std::unordered_set<ChunkPos, ChunkPosHash> *out) const
{
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);

    out->clear();
    out->reserve(m_chunks.size());

    for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it)
    {
        out->insert(it->first);
    }
}

size_t Level::GetChunkAllocatedBytes() const
{
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);

    size_t bytes = 0;
    for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it)
    {
        bytes += it->second->GetAllocatedBytes();
    }

    return bytes;
}
size_t Level::GetLoadedChunkCount() const
{
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
    return m_chunks.size();
}

void Level::MarkChunkDirty(const BlockPos &pos)
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);

    int cx = Mth::FloorDiv(pos.x, Chunk::s_sizeX);
    int cz = Mth::FloorDiv(pos.z, Chunk::s_sizeZ);

    ChunkPos chunkPos{cx, cz};
    if (m_dirtyChunksSet.insert(chunkPos).second)
    {
        m_dirtyChunks.push_back(chunkPos);
    }
}

void Level::MarkChunkDirtyUrgent(const ChunkPos &pos)
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    if (m_urgentDirtyChunksSet.insert(pos).second)
    {
        m_urgentDirtyChunks.push_front(pos);
    }
}

void Level::ClearDirtyChunks()
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);

    m_dirtyChunks.clear();
    m_dirtyChunksSet.clear();
    m_urgentDirtyChunks.clear();
    m_urgentDirtyChunksSet.clear();
    m_urgentDirtySections.clear();
    m_urgentDirtySectionsSet.clear();
}

const std::deque<ChunkPos> &Level::GetDirtyChunks() const
{
    return m_dirtyChunks;
}

size_t Level::GetQueuedDirtyChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    return m_dirtyChunks.size();
}

size_t Level::GetUrgentDirtyChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    return m_urgentDirtyChunks.size() + m_urgentDirtySections.size();
}

Chunk *Level::FindChunk(const ChunkPos &pos) const
{
    uint64_t epoch = m_removeEpoch.load(std::memory_order_acquire);
    if (s_chunkCache.levelId != m_id || s_chunkCache.epoch != epoch)
    {
        s_chunkCache.levelId = m_id;
        s_chunkCache.epoch   = epoch;
        for (int i = 0; i < kChunkCacheSize; i++)
        {
            s_chunkCache.chunks[i] = nullptr;
        }
    }

    int slot = (int) (((uint32_t) pos.x * 73856093u) ^ ((uint32_t) pos.z * 19349663u)) & (kChunkCacheSize - 1);
    if (s_chunkCache.chunks[slot] && s_chunkCache.positions[slot] == pos)
    {
        return s_chunkCache.chunks[slot];
    }

    Chunk *chunk = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        auto it = m_chunks.find(pos);
        if (it != m_chunks.end())
        {
            chunk = it->second.get();
        }
    }

    if (chunk)
    {
        s_chunkCache.positions[slot] = pos;
        s_chunkCache.chunks[slot]    = chunk;
    }

    return chunk;
}

std::unique_ptr<Chunk> Level::RemoveChunk(const ChunkPos &pos)
{
    std::unique_lock<std::shared_mutex> lock(m_chunksMutex);

    auto it = m_chunks.find(pos);
    if (it == m_chunks.end())
    {
        return nullptr;
    }

    std::unique_ptr<Chunk> chunk = std::move(it->second);
    m_chunks.erase(it);
    m_removeEpoch.fetch_add(1, std::memory_order_release);
    return chunk;
}

bool Level::InsertChunk(const ChunkPos &pos, std::unique_ptr<Chunk> chunk)
{
    if (!chunk)
    {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(m_chunksMutex);

    chunk->SetPos(pos);

    auto result = m_chunks.emplace(pos, std::move(chunk));
    if (!result.second)
    {
        return false;
    }

    return true;
}

void Level::ClearMeshDirtyQueues()
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    std::deque<ChunkPos>().swap(m_dirtyChunks);
    std::deque<ChunkPos>().swap(m_urgentDirtyChunks);
    std::deque<std::vector<SectionPos>>().swap(m_urgentDirtySections);
    m_dirtyChunksSet.clear();
    m_urgentDirtyChunksSet.clear();
    m_urgentDirtySectionsSet.clear();
}

void Level::MarkSectionsDirtyUrgent(const std::vector<SectionPos> &positions)
{
    std::vector<SectionPos> group;
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    for (size_t i = 0; i < positions.size(); i++)
    {
        if (m_urgentDirtySectionsSet.insert(positions[i]).second)
        {
            group.push_back(positions[i]);
        }
    }

    if (!group.empty())
    {
        m_urgentDirtySections.push_front(std::move(group));
    }
}

bool Level::PollDirtyChunk(ChunkPos *outPos, const ChunkPos &center)
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    if (m_dirtyChunks.empty())
    {
        return false;
    }

    size_t nearest         = FindNearest(m_dirtyChunks, center);
    *outPos                = m_dirtyChunks[nearest];
    m_dirtyChunks[nearest] = m_dirtyChunks.back();
    m_dirtyChunks.pop_back();
    m_dirtyChunksSet.erase(*outPos);
    return true;
}

bool Level::PollUrgentDirtyChunk(ChunkPos *outPos)
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    if (m_urgentDirtyChunks.empty())
    {
        return false;
    }

    *outPos = m_urgentDirtyChunks.front();
    m_urgentDirtyChunks.pop_front();
    m_urgentDirtyChunksSet.erase(*outPos);
    return true;
}

bool Level::PollUrgentDirtySections(std::vector<SectionPos> *outPositions)
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    if (m_urgentDirtySections.empty())
    {
        return false;
    }

    *outPositions = std::move(m_urgentDirtySections.front());
    m_urgentDirtySections.pop_front();
    for (size_t i = 0; i < outPositions->size(); i++)
    {
        m_urgentDirtySectionsSet.erase((*outPositions)[i]);
    }

    return true;
}

void Level::AddEntity(std::unique_ptr<Entity> entity)
{
    m_entitiesById[entity->GetId()] = entity.get();
    if (entity->IsSolid())
    {
        m_solidEntities.push_back(entity.get());
    }

    if (m_tickingEntities)
    {
        m_pendingAddedEntities.push_back(std::move(entity));
        return;
    }

    m_entities.push_back(std::move(entity));
}

void Level::RemoveEntity(Entity *entity)
{
    if (m_tickingEntities)
    {
        m_pendingRemovedEntities.push_back(entity);
        return;
    }

    EraseEntityId(entity);
    m_solidEntities.erase(std::remove(m_solidEntities.begin(), m_solidEntities.end(), entity), m_solidEntities.end());
    m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(),
                                    [entity](const std::unique_ptr<Entity> &ptr)
                                    {
                                        return ptr.get() == entity;
                                    }),
                     m_entities.end());
}

// This sorts the given list and skips the per entity hooks, so callers record removals themselves
void Level::RemoveEntities(std::vector<Entity *> *entities)
{
    if (entities->empty())
    {
        return;
    }

    for (size_t i = 0; i < entities->size(); i++)
    {
        EraseEntityId((*entities)[i]);
    }

    std::sort(entities->begin(), entities->end());
    m_solidEntities.erase(std::remove_if(m_solidEntities.begin(), m_solidEntities.end(),
                                         [entities](Entity *entity)
                                         {
                                             return std::binary_search(entities->begin(), entities->end(), entity);
                                         }),
                          m_solidEntities.end());
    m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(),
                                    [entities](const std::unique_ptr<Entity> &ptr)
                                    {
                                        return std::binary_search(entities->begin(), entities->end(), ptr.get());
                                    }),
                     m_entities.end());
}

void Level::TickEntities()
{
    m_tickingEntities = true;

    // Packets land during the local player's tick, so every old position is stored first to keep the move a packet applies visible
    for (std::unique_ptr<Entity> &entity : m_entities)
    {
        entity->StoreOld();
    }

    for (std::unique_ptr<Entity> &entity : m_entities)
    {
        // Riders tick right after their vehicle so they sit where it ended up, server players are ticked by the player list instead
        if (entity->GetVehicle() && (IsClientSide() || !dynamic_cast<Player *>(entity.get())))
        {
            continue;
        }

        const glm::dvec3 &position = entity->GetPosition();
        // A non-finite entity is saved with its chunk and spreads NaN to every client that touches it, so we drop it
        if (!IsClientSide() && !dynamic_cast<Player *>(entity.get()) &&
            (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)))
        {
            Logger::LogWarn("Removed entity with non-finite position type=%d", (int) entity->GetType());
            RemoveEntity(entity.get());
            continue;
        }

        // Lightning has to tick everywhere because it only removes itself in its own tick, so a far bolt would hang forever
        if (!dynamic_cast<Player *>(entity.get()) && entity->GetType() != eEntityType_LightningBolt &&
            ((!IsClientSide() && !IsInSimulationDistance(position)) ||
             !IsAreaLoaded(BlockPos((int) floor(position.x), 0, (int) floor(position.z)), 1)))
        {
            continue;
        }

        if (GetTickRateManager()->IsEntityFrozen(entity.get()))
        {
            continue;
        }

        TickProfilerEntityScope profilerScope(entity->GetType());
        entity->Tick();
        for (size_t i = 0; i < entity->GetPassengers().size(); i++)
        {
            Entity *passenger = entity->GetPassengers()[i];
            if (IsClientSide() || !dynamic_cast<Player *>(passenger))
            {
                passenger->Tick();
            }
        }
    }

    m_tickingEntities = false;
    for (std::unique_ptr<Entity> &entity : m_pendingAddedEntities)
    {
        m_entities.push_back(std::move(entity));
    }

    m_pendingAddedEntities.clear();
    Level::RemoveEntities(&m_pendingRemovedEntities);
    m_pendingRemovedEntities.clear();
}

const std::vector<std::unique_ptr<Entity>> &Level::GetEntities() const
{
    return m_entities;
}

const std::vector<Entity *> &Level::GetSolidEntities() const
{
    return m_solidEntities;
}

Entity *Level::GetEntityById(int id) const
{
    auto it = m_entitiesById.find(id);
    return it == m_entitiesById.end() ? nullptr : it->second;
}

Player *Level::FindNearestPlayer(const glm::dvec3 &position, double maxDistance) const
{
    Player *nearest      = nullptr;
    double nearestDistSq = maxDistance * maxDistance;
    for (size_t i = 0; i < m_entities.size(); i++)
    {
        Player *player = dynamic_cast<Player *>(m_entities[i].get());
        if (!player || GameType::IsSpectator(player->GetGameType()) || player->GetHealth() <= 0)
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

void Level::GetPushableEntities(const AABB &box, const Entity *except, std::vector<Entity *> *entities) const
{
    for (size_t i = 0; i < m_entities.size(); i++)
    {
        LivingEntity *entity = dynamic_cast<LivingEntity *>(m_entities[i].get());
        if (entity && entity != except && entity->GetHealth() > 0 && !entity->GetSharedFlag(eEntityFlag_Spectator) &&
            entity->GetAABB().Intersects(box))
        {
            entities->push_back(entity);
        }
    }
}

void Level::SpawnParticle(ePARTICLE_TYPE type, const glm::dvec3 &position)
{
    SpawnParticle(type, position, glm::dvec3(), 1);
}

void Level::SpawnParticle(ePARTICLE_TYPE type, const glm::dvec3 &position, int amount)
{
    SpawnParticle(type, position, glm::dvec3(), amount);
}

void Level::SpawnParticle(ePARTICLE_TYPE type, const glm::dvec3 &position, const glm::dvec3 &spread, int amount)
{
    ParticleSpawnParams params;
    params.type     = type;
    params.position = position;
    params.spread   = spread;
    params.amount   = amount;
    SpawnParticle(params);
}

void Level::SpawnParticle(const ParticleSpawnParams &)
{}

void Level::PlaySound(const ResourceLocation &, const glm::dvec3 &, float, float)
{}

void Level::PlaySound(const ResourceLocation &, float, float)
{}

void Level::PlayThunder(const glm::dvec3 &, float, float)
{}

Player *Level::CreatePlayer(const std::wstring &name)
{
    return new Player(this, name);
}

Player *Level::GetLocalPlayer() const
{
    return nullptr;
}

void Level::OnSectionMeshDirty(const SectionPos &)
{}

void Level::OnBlockLightChanged(const std::unordered_set<SectionPos, SectionPosHash> &, bool)
{}

bool Level::IsChunkMeshBusy(const ChunkPos &) const
{
    return false;
}

void Level::ReleaseChunkMeshes(const ChunkPos &)
{}

bool Level::IsClipTarget(const Entity *entity) const
{
    return entity->IsPickable();
}

void Level::PlayBlockBreakSound(const BlockPos &pos, Block *block)
{
    if (!block)
    {
        return;
    }

    const std::string &breakSound = block->GetBreakSound();
    if (breakSound.empty())
    {
        return;
    }

    int variant      = m_random.NextInt(1, block->GetBreakSoundVariants());
    std::string name = breakSound + std::to_string(variant);
    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    PlaySound(name, center, 1.0f, s_breakSoundPitch);
}

void Level::PlayBlockPlaceSound(const BlockPos &pos, Block *block)
{
    if (!block)
    {
        return;
    }

    const std::string &placeSound = block->GetPlaceSound();
    if (placeSound.empty())
    {
        return;
    }

    std::string name =
            block->GetPlaceSoundVariants() > 0 ? placeSound + std::to_string(m_random.NextInt(1, block->GetPlaceSoundVariants())) : placeSound;
    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    PlaySound(name, center, s_placeSoundVolume, block->GetPlaceSoundPitch());
}

uint32_t Level::GetBlockId(const BlockPos &pos) const
{
    int cx = Mth::FloorDiv(pos.x, Chunk::s_sizeX);
    int cz = Mth::FloorDiv(pos.z, Chunk::s_sizeZ);

    int lx = Mth::FloorMod(pos.x, Chunk::s_sizeX);
    int ly = Mth::FloorMod(pos.y, Chunk::s_sizeY);
    int lz = Mth::FloorMod(pos.z, Chunk::s_sizeZ);
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return 0;
    }

    const Chunk *chunk = GetChunk(ChunkPos{cx, cz});
    if (!chunk)
    {
        return 0;
    }

    return chunk->GetBlockId(lx, ly, lz);
}

uint16_t Level::GetBlockData(const BlockPos &pos) const
{
    int ly = Mth::FloorMod(pos.y, Chunk::s_sizeY);
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return 0;
    }

    const Chunk *chunk = GetChunk(ChunkPos{Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)});
    if (!chunk)
    {
        return 0;
    }

    return chunk->GetBlockData(Mth::FloorMod(pos.x, Chunk::s_sizeX), ly, Mth::FloorMod(pos.z, Chunk::s_sizeZ));
}

bool Level::IsClientSide() const
{
    return false;
}

GameRules &Level::GetGameRules()
{
    return *m_gameRules;
}

void Level::ShareGameRules(Level *level)
{
    m_gameRules = level->m_gameRules;
}

Scoreboard &Level::GetScoreboard()
{
    return *m_scoreboard;
}

void Level::ShareScoreboard(Level *level)
{
    m_scoreboard = level->m_scoreboard;
}

void Level::RecordLiquidChange(const BlockPos &pos, Block *oldBlock, Block *newBlock)
{
    if (oldBlock->IsLiquid() || newBlock->IsLiquid())
    {
        RecordBlockChange(pos);
    }
}

void Level::RecordBlockChange(const BlockPos &)
{}

void Level::OnBlockSet(const BlockPos &pos)
{
    Block *block     = Block::ById(GetBlockId(pos));
    const char *id   = block ? block->GetBlockEntityId() : nullptr;
    auto blockEntity = m_blockEntities.find(pos);
    if (blockEntity != m_blockEntities.end() && (!id || strcmp(blockEntity->second->GetId(), id) != 0))
    {
        m_blockEntities.erase(blockEntity);
    }

    if (id)
    {
        GetBlockEntity(pos);
    }
}

BlockEntity *Level::GetBlockEntity(const BlockPos &pos)
{
    std::unique_ptr<BlockEntity> &blockEntity = m_blockEntities[pos];
    if (!blockEntity)
    {
        Block *block = Block::ById(GetBlockId(pos));
        if (block && block->GetBlockEntityId())
        {
            blockEntity = BlockEntity::Create(block->GetBlockEntityId(), this, pos);
            blockEntity->SetTickOrder(m_nextBlockEntityTickOrder);
            m_blockEntityTickOrder.push_back({pos, m_nextBlockEntityTickOrder});
            m_nextBlockEntityTickOrder++;
        }
        else
        {
            m_blockEntities.erase(pos);
            return nullptr;
        }
    }

    return blockEntity.get();
}

const std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> &Level::GetBlockEntities() const
{
    return m_blockEntities;
}

void Level::TickBlockEntities()
{
    // We tick in creation order because piston timing depends on it, and ticking can add or remove entries so we look each one up again
    size_t count = m_blockEntityTickOrder.size();
    size_t kept  = 0;
    for (size_t i = 0; i < count; i++)
    {
        std::pair<BlockPos, uint64_t> entry = m_blockEntityTickOrder[i];
        auto blockEntity                    = m_blockEntities.find(entry.first);
        if (blockEntity == m_blockEntities.end() || blockEntity->second->GetTickOrder() != entry.second)
        {
            continue;
        }

        blockEntity->second->Tick();
        m_blockEntityTickOrder[kept] = entry;
        kept++;
    }

    m_blockEntityTickOrder.erase(m_blockEntityTickOrder.begin() + kept, m_blockEntityTickOrder.begin() + count);
}

void Level::SetBlockData(const BlockPos &pos, uint16_t data)
{
    Chunk *chunk     = GetChunk(ChunkPos{Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)});
    uint16_t oldData = GetBlockData(pos);
    if (pos.y >= 0 && pos.y < Chunk::s_sizeY && chunk)
    {
        chunk->SetBlockData(Mth::FloorMod(pos.x, Chunk::s_sizeX), pos.y, Mth::FloorMod(pos.z, Chunk::s_sizeZ), data);
    }

    Block *block = Block::ById(GetBlockId(pos));
    RecordLiquidChange(pos, block, block);
    if (oldData != data)
    {
        NotifyNeighborStates(pos);
    }

    MarkBlockMeshDirty(pos, true);
}

void Level::SetBlock(const BlockPos &pos, Block *block)
{
    SetBlock(pos, block, nullptr);
}

void Level::SetBlock(const BlockPos &pos, Block *block, Direction *placedAgainst)
{
    SetBlock(pos, block, placedAgainst, block->GetPlacementData(placedAgainst));
}

void Level::SetBlock(const BlockPos &pos, Block *block, Direction *placedAgainst, uint16_t data)
{
    (void) placedAgainst;
    SetBlockAndData(pos, block, data, true);
}

void Level::SetBlockDeferredMesh(const BlockPos &pos, Block *block, uint16_t data)
{
    SetBlockAndData(pos, block, data, false);
}

void Level::SetBlockAndData(const BlockPos &pos, Block *block, uint16_t data, bool urgentMesh)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return;
    }

    int cx = Mth::FloorDiv(pos.x, Chunk::s_sizeX);
    int cz = Mth::FloorDiv(pos.z, Chunk::s_sizeZ);

    int lx = Mth::FloorMod(pos.x, Chunk::s_sizeX);
    int ly = Mth::FloorMod(pos.y, Chunk::s_sizeY);
    int lz = Mth::FloorMod(pos.z, Chunk::s_sizeZ);

    ChunkPos chunkPos{cx, cz};
    Chunk *chunk = GetChunk(chunkPos);
    if (!chunk)
    {
        return;
    }

    Block *oldBlock  = Block::ById(chunk->GetBlockId(lx, ly, lz));
    uint16_t oldData = chunk->GetBlockData(lx, ly, lz);
    bool runLogic    = !IsClientSide() && oldBlock != block;
    if (runLogic)
    {
        oldBlock->OnBreak(this, pos);
    }

    chunk->SetBlock(lx, ly, lz, block);

    chunk->SetBlockData(lx, ly, lz, data);
    RecordLiquidChange(pos, oldBlock, block);
    OnBlockSet(pos);
    if (oldBlock != block || oldData != data)
    {
        NotifyNeighborStates(pos);
    }

    if (runLogic)
    {
        oldBlock->OnRemoved(this, pos, oldData);
        block->OnPlace(this, pos);
        NotifyBlockNeighbors(pos);
        if (LeavesBlock::IsLog(oldBlock))
        {
            LeavesBlock::FlagDecayAround(this, pos);
        }

        if (block->GetLightEmission() > IceBlock::s_meltLightLevel)
        {
            ((PointedDripstoneBlock *) Block::ByName("icicle"))->OnHeatPlaced(this, pos, block->GetLightEmission());
        }
    }
    else if (oldBlock != block && IsClientSide())
    {
        block->OnClientPlace(this, pos);
        for (uint8_t i = 1; i <= 6; i++)
        {
            Direction *direction = Direction::ById(i);
            BlockPos neighbor(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz);
            if (neighbor.y >= 0 && neighbor.y < Chunk::s_sizeY &&
                HasChunk(ChunkPos(Mth::FloorDiv(neighbor.x, Chunk::s_sizeX), Mth::FloorDiv(neighbor.z, Chunk::s_sizeZ))))
            {
                Block::ById(GetBlockId(neighbor))->UpdateShape(this, neighbor);
            }
        }
    }

    QueueLightUpdateIfChanged(pos, oldBlock, block);
    MarkBlockMeshDirty(pos, urgentMesh);
}

void Level::SetBlockAndDataNoNeighborUpdates(const BlockPos &pos, Block *block, uint16_t data)
{
    Chunk *chunk    = GetChunk(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)));
    Block *oldBlock = Block::ById(chunk->GetBlockId(Mth::FloorMod(pos.x, Chunk::s_sizeX), pos.y, Mth::FloorMod(pos.z, Chunk::s_sizeZ)));
    RecordLiquidChange(pos, oldBlock, block);
    chunk->SetBlock(Mth::FloorMod(pos.x, Chunk::s_sizeX), pos.y, Mth::FloorMod(pos.z, Chunk::s_sizeZ), block);
    chunk->SetBlockData(Mth::FloorMod(pos.x, Chunk::s_sizeX), pos.y, Mth::FloorMod(pos.z, Chunk::s_sizeZ), data);
    OnBlockSet(pos);
    NotifyNeighborStates(pos);
    QueueLightUpdateIfChanged(pos, oldBlock, block);
    MarkBlockMeshDirty(pos, true);
}

void Level::NotifyNeighborStates(const BlockPos &pos)
{
    if (IsClientSide())
    {
        return;
    }

    for (uint8_t i = 1; i <= 6; i++)
    {
        Direction *direction = Direction::ById(i);
        BlockPos neighbor(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz);
        if (neighbor.y >= 0 && neighbor.y < Chunk::s_sizeY &&
            HasChunk(ChunkPos(Mth::FloorDiv(neighbor.x, Chunk::s_sizeX), Mth::FloorDiv(neighbor.z, Chunk::s_sizeZ))))
        {
            Block::ById(GetBlockId(neighbor))->OnNeighborStateChanged(this, neighbor, Direction::Opposite(direction));
        }
    }
}

void Level::NotifyBlockNeighbors(const BlockPos &pos)
{
    m_neighborUpdates.push_back(BlockPos(pos.x - 1, pos.y, pos.z));
    m_neighborUpdates.push_back(BlockPos(pos.x + 1, pos.y, pos.z));
    m_neighborUpdates.push_back(BlockPos(pos.x, pos.y - 1, pos.z));
    m_neighborUpdates.push_back(BlockPos(pos.x, pos.y + 1, pos.z));
    m_neighborUpdates.push_back(BlockPos(pos.x, pos.y, pos.z - 1));
    m_neighborUpdates.push_back(BlockPos(pos.x, pos.y, pos.z + 1));
    RunNeighborUpdates();
}

void Level::NotifyBlockAndNeighbors(const BlockPos &pos, const BlockPos &except)
{
    m_neighborUpdates.push_back(pos);
    BlockPos neighbors[] = {BlockPos(pos.x - 1, pos.y, pos.z), BlockPos(pos.x + 1, pos.y, pos.z), BlockPos(pos.x, pos.y - 1, pos.z),
                            BlockPos(pos.x, pos.y + 1, pos.z), BlockPos(pos.x, pos.y, pos.z - 1), BlockPos(pos.x, pos.y, pos.z + 1)};
    for (int i = 0; i < 6; i++)
    {
        if (neighbors[i] != except)
        {
            m_neighborUpdates.push_back(neighbors[i]);
        }
    }

    RunNeighborUpdates();
}

void Level::RunNeighborUpdates()
{
    // Updates queue up instead of recursing because a large field of unsupported blocks breaking in a chain overflows the stack
    if (m_updatingNeighbors)
    {
        return;
    }

    m_updatingNeighbors = true;
    for (size_t i = 0; i < m_neighborUpdates.size(); i++)
    {
        BlockPos neighbor = m_neighborUpdates[i];
        if (neighbor.y < 0 || neighbor.y >= Chunk::s_sizeY)
        {
            continue;
        }

        if (!HasChunk(ChunkPos(Mth::FloorDiv(neighbor.x, Chunk::s_sizeX), Mth::FloorDiv(neighbor.z, Chunk::s_sizeZ))))
        {
            continue;
        }

        Block::ById(GetBlockId(neighbor))->OnNeighborChanged(this, neighbor);
    }

    m_neighborUpdates.clear();
    m_updatingNeighbors = false;
}

int Level::GetDirectSignalTo(const BlockPos &pos) const
{
    int signal = 0;
    for (uint8_t i = 1; i <= 6; i++)
    {
        Direction *direction = Direction::ById(i);
        BlockPos neighbor(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz);
        signal = std::max(signal, Block::ById(GetBlockId(neighbor))->GetDirectSignal(GetBlockData(neighbor), Direction::Opposite(direction)));
    }

    return signal;
}

int Level::GetSignal(const BlockPos &pos, Direction *direction) const
{
    Block *block = Block::ById(GetBlockId(pos));
    if (block->IsSignalConductor())
    {
        return GetDirectSignalTo(pos);
    }

    return block->GetSignal(GetBlockData(pos), direction);
}

int Level::GetBestNeighborSignal(const BlockPos &pos) const
{
    int signal = 0;
    for (uint8_t i = 1; i <= 6; i++)
    {
        Direction *direction = Direction::ById(i);
        signal = std::max(signal,
                          GetSignal(BlockPos(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz), Direction::Opposite(direction)));
    }

    return signal;
}

void Level::MarkBlockMeshDirty(const BlockPos &pos, bool urgent)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return;
    }

    int cx            = Mth::FloorDiv(pos.x, Chunk::s_sizeX);
    int cz            = Mth::FloorDiv(pos.z, Chunk::s_sizeZ);
    int lx            = Mth::FloorMod(pos.x, Chunk::s_sizeX);
    int ly            = Mth::FloorMod(pos.y, Chunk::s_sizeY);
    int lz            = Mth::FloorMod(pos.z, Chunk::s_sizeZ);
    int sectionY      = ly / ChunkSection::s_size;
    int localSectionY = ly - sectionY * ChunkSection::s_size;
    int minSectionY   = sectionY;
    int maxSectionY   = sectionY;
    if (localSectionY == 0 && sectionY > 0)
    {
        minSectionY = sectionY - 1;
    }

    if (localSectionY == ChunkSection::s_size - 1 && sectionY < Chunk::s_sectionCount - 1)
    {
        maxSectionY = sectionY + 1;
    }

    std::vector<SectionPos> urgentSections;
    for (int dz = -1; dz <= 1; dz++)
    {
        if ((dz < 0 && lz != 0) || (dz > 0 && lz != Chunk::s_sizeZ - 1))
        {
            continue;
        }

        for (int dx = -1; dx <= 1; dx++)
        {
            if ((dx < 0 && lx != 0) || (dx > 0 && lx != Chunk::s_sizeX - 1))
            {
                continue;
            }

            ChunkPos affectedChunkPos(cx + dx, cz + dz);
            if ((dx != 0 || dz != 0) && !HasChunk(affectedChunkPos))
            {
                continue;
            }

            for (int affectedSectionY = minSectionY; affectedSectionY <= maxSectionY; affectedSectionY++)
            {
                SectionPos sectionPos(affectedChunkPos.x, affectedSectionY, affectedChunkPos.z);
                if (urgent)
                {
                    urgentSections.push_back(sectionPos);
                }
                else
                {
                    OnSectionMeshDirty(sectionPos);
                }
            }
        }
    }

    if (urgent)
    {
        MarkSectionsDirtyUrgent(urgentSections);
    }
}

void Level::ScheduleWaterTicks(const ChunkPos &pos)
{
    Chunk *chunk = GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    const std::vector<uint16_t> &liquidTicks = chunk->TakeLiquidTicks();

    int index = 0;
    for (auto liquidTickIt = liquidTicks.begin(); liquidTickIt != liquidTicks.end(); ++liquidTickIt)
    {
        uint16_t packed = *liquidTickIt;
        int x           = packed & 15;
        int z           = (packed >> 4) & 15;
        int y           = packed >> 8;

        ScheduleBlockForTick(BlockPos(pos.x * Chunk::s_sizeX + x, y, pos.z * Chunk::s_sizeZ + z), (uint32_t) (5 + (index++ & 15)), 0);
    }

    chunk->ClearLiquidTicks();
}

void Level::ScheduleSavedBlockTicks(const ChunkPos &pos)
{
    Chunk *chunk = GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    std::vector<ChunkBlockTick> blockTicks = chunk->TakeBlockTicks();
    for (size_t i = 0; i < blockTicks.size(); i++)
    {
        const ChunkBlockTick &tick = blockTicks[i];
        BlockPos blockPos(pos.x * Chunk::s_sizeX + tick.x, tick.y, pos.z * Chunk::s_sizeZ + tick.z);
        if (Block::ById(GetBlockId(blockPos)) == tick.block)
        {
            ScheduleBlockForTick(blockPos, (uint32_t) std::max(tick.delay, 0), tick.priority);
        }
    }
}

void Level::ScheduleBlockForTick(const BlockPos &, uint32_t, int)
{}

void Level::AddBlockEvent(const BlockPos &, Block *, int)
{}

void Level::StoreBlockTicks(Chunk *, bool)
{}

void Level::StoreBlockEntities(Chunk *chunk, bool remove)
{
    const ChunkPos &chunkPos = chunk->GetPos();
    std::vector<std::shared_ptr<CompoundTag>> tags;
    for (auto blockEntity = m_blockEntities.begin(); blockEntity != m_blockEntities.end();)
    {
        const BlockPos &pos = blockEntity->first;
        if (Mth::FloorDiv(pos.x, Chunk::s_sizeX) != chunkPos.x || Mth::FloorDiv(pos.z, Chunk::s_sizeZ) != chunkPos.z)
        {
            blockEntity++;
            continue;
        }

        std::shared_ptr<CompoundTag> tag(new CompoundTag());
        blockEntity->second->Save(tag.get());
        tags.push_back(tag);
        blockEntity = remove ? m_blockEntities.erase(blockEntity) : std::next(blockEntity);
    }

    chunk->SetBlockEntities(std::move(tags));
}

void Level::RemoveBlockEntities(const ChunkPos &pos)
{
    for (auto blockEntity = m_blockEntities.begin(); blockEntity != m_blockEntities.end();)
    {
        if (Mth::FloorDiv(blockEntity->first.x, Chunk::s_sizeX) == pos.x && Mth::FloorDiv(blockEntity->first.z, Chunk::s_sizeZ) == pos.z)
        {
            blockEntity = m_blockEntities.erase(blockEntity);
        }
        else
        {
            blockEntity++;
        }
    }
}

void Level::AdoptBlockEntities(const ChunkPos &pos)
{
    std::vector<std::shared_ptr<CompoundTag>> tags = GetChunk(pos)->TakeBlockEntities();
    for (size_t i = 0; i < tags.size(); i++)
    {
        const CompoundTag *tag   = tags[i].get();
        BlockEntity *blockEntity = GetBlockEntity(BlockPos(tag->GetInt("x"), tag->GetInt("y"), tag->GetInt("z")));
        if (blockEntity && tag->GetString("id") == blockEntity->GetId())
        {
            blockEntity->Load(tag);
        }
    }
}

size_t Level::GetScheduledBlockTickCount() const
{
    return 0;
}

bool Level::IsAreaLoaded(const BlockPos &pos, int radius) const
{
    for (int chunkX = Mth::FloorDiv(pos.x - radius, Chunk::s_sizeX); chunkX <= Mth::FloorDiv(pos.x + radius, Chunk::s_sizeX); chunkX++)
    {
        for (int chunkZ = Mth::FloorDiv(pos.z - radius, Chunk::s_sizeZ); chunkZ <= Mth::FloorDiv(pos.z + radius, Chunk::s_sizeZ); chunkZ++)
        {
            if (!HasChunk(ChunkPos(chunkX, chunkZ)))
            {
                return false;
            }
        }
    }

    return true;
}

WorldBorder &Level::GetWorldBorder()
{
    return m_worldBorder;
}

DynamicLights &Level::GetDynamicLights()
{
    return m_dynamicLights;
}

void Level::SetDifficulty(int difficulty)
{
    if (difficulty < 0)
    {
        difficulty = 0;
    }

    if (difficulty > 3)
    {
        difficulty = 3;
    }

    m_difficulty = difficulty;
}

int Level::GetDifficulty() const
{
    return m_difficulty;
}

void Level::SetDefaultGameType(eGAME_TYPE gameType)
{
    m_defaultGameType = gameType;
}

eGAME_TYPE Level::GetDefaultGameType() const
{
    return m_defaultGameType;
}

void Level::SetOnlineGame(bool onlineGame)
{
    m_onlineGame = onlineGame;
}

bool Level::IsOnlineGame() const
{
    return m_onlineGame;
}

void Level::SetAllowFriendsOfFriends(bool allowFriendsOfFriends)
{
    m_allowFriendsOfFriends = allowFriendsOfFriends;
}

bool Level::AllowsFriendsOfFriends() const
{
    return m_allowFriendsOfFriends;
}

void Level::SetMaxPlayers(int maxPlayers)
{
    m_maxPlayers = maxPlayers;
}

int Level::GetMaxPlayers() const
{
    return m_maxPlayers;
}

void Level::SetEmptyChunksSolid(bool solid)
{
    m_emptyChunksSolid = solid;
}

bool Level::AreEmptyChunksSolid() const
{
    return m_emptyChunksSolid;
}

void Level::SetEmptyChunksSolidForMeshing(bool solid)
{
    m_emptyChunksSolidForMeshing = solid;
}

bool Level::AreEmptyChunksSolidForMeshing() const
{
    return m_emptyChunksSolidForMeshing;
}

int Level::GetSurfaceHeight(int levelX, int levelZ) const
{
    int chunkX = (int) floor((double) levelX / Chunk::s_sizeX);
    int chunkZ = (int) floor((double) levelZ / Chunk::s_sizeZ);
    int localX = levelX - chunkX * Chunk::s_sizeX;
    int localZ = levelZ - chunkZ * Chunk::s_sizeZ;

    ChunkPos pos{chunkX, chunkZ};

    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);

    auto it = m_chunks.find(pos);
    if (it == m_chunks.end())
    {
        return 0;
    }

    const Chunk &chunk = *it->second;

    for (int y = Chunk::s_sizeY - 1; y >= 0; y--)
    {
        if (Block::ById(chunk.GetBlockId(localX, y, localZ))->IsSolid())
        {
            return y;
        }
    }

    return 0;
}

void Level::GetCollisionBoxes(const AABB &aabb, std::vector<AABB> *boxes) const
{
    thread_local std::vector<AABB> blockBoxes;

    boxes->clear();
    int minX = (int) floor(aabb.GetMin().x);
    // One block lower so boxes taller than a block, like fences, are found from above
    int minY = (int) floor(aabb.GetMin().y) - 1;
    int minZ = (int) floor(aabb.GetMin().z);

    int maxX = (int) floor(aabb.GetMax().x);
    int maxY = (int) floor(aabb.GetMax().y);
    int maxZ = (int) floor(aabb.GetMax().z);

    for (int z = minZ; z <= maxZ; z++)
    {
        for (int y = minY; y <= maxY; y++)
        {
            if (y < 0 || y >= Chunk::s_sizeY)
            {
                continue;
            }

            for (int x = minX; x <= maxX; x++)
            {
                const Chunk *chunk = GetChunk({Mth::FloorDiv(x, Chunk::s_sizeX), Mth::FloorDiv(z, Chunk::s_sizeZ)});
                if (!chunk)
                {
                    if (m_emptyChunksSolid)
                    {
                        boxes->push_back(AABB(glm::dvec3(x, y, z), glm::dvec3(x + 1, y + 1, z + 1)));
                    }

                    continue;
                }

                int lx       = Mth::FloorMod(x, Chunk::s_sizeX);
                int lz       = Mth::FloorMod(z, Chunk::s_sizeZ);
                Block *block = Block::ById(chunk->GetBlockId(lx, y, lz));
                if (!block->IsCollidable())
                {
                    continue;
                }

                if (block->GetRenderShape() == Block::eRenderShape_CUBE)
                {
                    boxes->push_back(AABB(glm::dvec3(x, y, z), glm::dvec3(x + 1, y + 1, z + 1)));
                    continue;
                }

                blockBoxes.clear();
                block->GetPlacedCollisionBoxes(this, BlockPos(x, y, z), chunk->GetBlockData(lx, y, lz), &blockBoxes);
                for (size_t i = 0; i < blockBoxes.size(); i++)
                {
                    boxes->push_back(blockBoxes[i].Translated(glm::dvec3(x, y, z)));
                }
            }
        }
    }
}

bool Level::IntersectsBlock(const AABB &aabb) const
{
    thread_local std::vector<AABB> collisionBoxes;

    int minX = (int) floor(aabb.GetMin().x);
    int minY = (int) floor(aabb.GetMin().y);
    int minZ = (int) floor(aabb.GetMin().z);

    int maxX = (int) floor(aabb.GetMax().x - 1e-6);
    int maxY = (int) floor(aabb.GetMax().y - 1e-6);
    int maxZ = (int) floor(aabb.GetMax().z - 1e-6);

    for (int z = minZ; z <= maxZ; z++)
    {
        for (int y = std::max(minY, 0); y <= std::min(maxY, Chunk::s_sizeY - 1); y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                int cx = Mth::FloorDiv(x, Chunk::s_sizeX);
                int cz = Mth::FloorDiv(z, Chunk::s_sizeZ);

                int lx = Mth::FloorMod(x, Chunk::s_sizeX);
                int ly = Mth::FloorMod(y, Chunk::s_sizeY);
                int lz = Mth::FloorMod(z, Chunk::s_sizeZ);

                const Chunk *chunk = GetChunk({cx, cz});
                if (!chunk)
                {
                    if (m_emptyChunksSolid)
                    {
                        return true;
                    }

                    continue;
                }

                Block *block = Block::ById(chunk->GetBlockId(lx, ly, lz));
                if (!block->IsCollidable())
                {
                    continue;
                }

                if (block->GetRenderShape() != Block::eRenderShape_CUBE)
                {
                    std::vector<AABB> &boxes = collisionBoxes;
                    boxes.clear();
                    block->GetCollisionBoxes(chunk->GetBlockData(lx, ly, lz), &boxes);
                    for (size_t i = 0; i < boxes.size(); i++)
                    {
                        if (aabb.Intersects(boxes[i].Translated(glm::dvec3(x, y, z))))
                        {
                            return true;
                        }
                    }

                    continue;
                }

                AABB blockBox(glm::dvec3(x, y, z), glm::dvec3(x + 1, y + 1, z + 1));
                if (aabb.Intersects(blockBox))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Level::ContainsLiquid(const AABB &aabb) const
{
    const glm::dvec3 &min = aabb.GetMin();
    const glm::dvec3 &max = aabb.GetMax();
    for (int x = (int) floor(min.x); x < (int) floor(max.x + 1.0); x++)
    {
        for (int y = (int) floor(min.y); y < (int) floor(max.y + 1.0); y++)
        {
            for (int z = (int) floor(min.z); z < (int) floor(max.z + 1.0); z++)
            {
                Block *block = Block::ById(GetBlockId(BlockPos(x, y, z)));
                if (block && block->IsLiquid())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

// A removal can land after a new entity took over the same id, so we only drop the id if it still points here
void Level::EraseEntityId(Entity *entity)
{
    auto it = m_entitiesById.find(entity->GetId());
    if (it != m_entitiesById.end() && it->second == entity)
    {
        m_entitiesById.erase(it);
    }
}

void Level::UpdateEntityId(Entity *entity, int oldId, int newId)
{
    auto it = m_entitiesById.find(oldId);
    if (it == m_entitiesById.end() || it->second != entity)
    {
        return;
    }

    m_entitiesById.erase(it);
    m_entitiesById[newId] = entity;
}

HitResult *Level::Clip(const glm::dvec3 &origin, const glm::dvec3 &direction, float maxDistance, bool includeEntities, bool stopOnLiquidSource,
                       bool collidableOnly)
{
    HitResult *result = new HitResult();
    result->SetMiss();

    glm::dvec3 normalizedDirection = glm::normalize(direction);

    float bestEntityT       = std::numeric_limits<float>::max();
    int bestEntityId        = 0;
    uint64_t bestEntityType = 0;
    bool hasEntity          = false;

    for (size_t i = 0; i < m_entities.size() && includeEntities; i++)
    {
        const std::unique_ptr<Entity> &entity = m_entities[i];
        if (!IsClipTarget(entity.get()))
        {
            continue;
        }

        const AABB &box = entity->GetAABB();
        glm::dvec3 min  = box.GetMin() - glm::dvec3(0.1, 0.1, 0.1);
        glm::dvec3 max  = box.GetMax() + glm::dvec3(0.1, 0.1, 0.1);

        float tMin = 0.0f;
        float tMax = maxDistance;
        bool valid = true;

        for (int axis = 0; axis < 3 && valid; axis++)
        {
            double o    = axis == 0 ? origin.x : (axis == 1 ? origin.y : origin.z);
            double d    = axis == 0 ? normalizedDirection.x : (axis == 1 ? normalizedDirection.y : normalizedDirection.z);
            double bMin = axis == 0 ? min.x : (axis == 1 ? min.y : min.z);
            double bMax = axis == 0 ? max.x : (axis == 1 ? max.y : max.z);

            if (d != 0.0)
            {
                float inv = (float) (1.0 / d);
                float t0  = (float) ((bMin - o) * inv);
                float t1  = (float) ((bMax - o) * inv);
                if (t0 > t1)
                {
                    std::swap(t0, t1);
                }

                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);
                if (tMax < tMin)
                {
                    valid = false;
                }
            }
            else if (o < bMin || o > bMax)
            {
                valid = false;
            }
        }

        if (valid && tMin >= 0.0f && tMin < bestEntityT)
        {
            bestEntityT    = tMin;
            bestEntityId   = entity->GetId();
            bestEntityType = (uint64_t) entity->GetType();
            hasEntity      = true;
        }
    }

    BlockPos pos((int) floor(origin.x), (int) floor(origin.y), (int) floor(origin.z));

    int stepX = normalizedDirection.x > 0.0f ? 1 : -1;
    int stepY = normalizedDirection.y > 0.0f ? 1 : -1;
    int stepZ = normalizedDirection.z > 0.0f ? 1 : -1;

    float tDeltaX = normalizedDirection.x != 0.0f ? (float) fabs(1.0 / normalizedDirection.x) : std::numeric_limits<float>::max();
    float tDeltaY = normalizedDirection.y != 0.0f ? (float) fabs(1.0 / normalizedDirection.y) : std::numeric_limits<float>::max();
    float tDeltaZ = normalizedDirection.z != 0.0f ? (float) fabs(1.0 / normalizedDirection.z) : std::numeric_limits<float>::max();

    double nextBoundaryX = stepX > 0 ? (double) pos.x + 1.0 : (double) pos.x;
    double nextBoundaryY = stepY > 0 ? (double) pos.y + 1.0 : (double) pos.y;
    double nextBoundaryZ = stepZ > 0 ? (double) pos.z + 1.0 : (double) pos.z;

    float tMaxX = normalizedDirection.x != 0.0f ? (float) ((nextBoundaryX - origin.x) / normalizedDirection.x) : std::numeric_limits<float>::max();
    float tMaxY = normalizedDirection.y != 0.0f ? (float) ((nextBoundaryY - origin.y) / normalizedDirection.y) : std::numeric_limits<float>::max();
    float tMaxZ = normalizedDirection.z != 0.0f ? (float) ((nextBoundaryZ - origin.z) / normalizedDirection.z) : std::numeric_limits<float>::max();

    if (tMaxX < 0.0f)
    {
        tMaxX = 0.0f;
    }

    if (tMaxY < 0.0f)
    {
        tMaxY = 0.0f;
    }

    if (tMaxZ < 0.0f)
    {
        tMaxZ = 0.0f;
    }

    float bestBlockT = std::numeric_limits<float>::max();
    BlockPos bestBlockPos;
    Direction *bestFace = nullptr;
    bool hasBlock       = false;
    float t             = 0.0f;
    bool originCell     = true;

    while (t <= maxDistance)
    {
        Direction *face = nullptr;

        // The cell holding the origin is tested first so a camera inside a block targets that block
        if (originCell)
        {
            originCell = false;
        }
        else if (tMaxX < tMaxY)
        {
            if (tMaxX < tMaxZ)
            {
                pos.x += stepX;
                t = tMaxX;
                tMaxX += tDeltaX;
                face = stepX > 0 ? Direction::west : Direction::east;
            }
            else
            {
                pos.z += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
                face = stepZ > 0 ? Direction::north : Direction::south;
            }
        }
        else
        {
            if (tMaxY < tMaxZ)
            {
                pos.y += stepY;
                t = tMaxY;
                tMaxY += tDeltaY;
                face = stepY > 0 ? Direction::down : Direction::up;
            }
            else
            {
                pos.z += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
                face = stepZ > 0 ? Direction::north : Direction::south;
            }
        }

        if (t > maxDistance)
        {
            break;
        }

        ChunkPos chunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ));

        int lx = Mth::FloorMod(pos.x, Chunk::s_sizeX);
        int lz = Mth::FloorMod(pos.z, Chunk::s_sizeZ);

        Chunk *chunk = GetChunk(chunkPos);
        if (!chunk)
        {
            continue;
        }

        if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
        {
            continue;
        }

        Block *block = Block::ById((int) chunk->GetBlockId(lx, pos.y, lz));
        if (!block)
        {
            continue;
        }

        uint16_t blockData = chunk->GetBlockData(lx, pos.y, lz);
        if (collidableOnly ? !block->IsCollidable() : !block->IsSelectable() && !(stopOnLiquidSource && block->IsLiquid() && blockData == 0))
        {
            continue;
        }

        std::vector<AABB> boxes;
        if (block->GetRenderShape() == Block::eRenderShape_BOXES)
        {
            block->GetPlacedBoxes(this, pos, blockData, &boxes);
            for (size_t i = 0; i < boxes.size(); i++)
            {
                boxes[i] = boxes[i].Translated(glm::dvec3(pos.x, pos.y, pos.z));
            }

            // The interaction box only swaps the face when it is hit first, so aiming into a bowl targets its top
            if (block->HasInteractionAABB())
            {
                boxes.push_back(block->GetInteractionAABB().Translated(glm::dvec3(pos.x, pos.y, pos.z)));
            }
        }
        else
        {
            boxes.push_back(block->GetPlacedAABB(pos, blockData));
        }

        float finalHitT      = std::numeric_limits<float>::max();
        Direction *finalFace = nullptr;
        for (size_t boxIndex = 0; boxIndex < boxes.size(); boxIndex++)
        {
            const AABB &blockBox  = boxes[boxIndex];
            const glm::dvec3 &min = blockBox.GetMin();
            const glm::dvec3 &max = blockBox.GetMax();

            if (max.x <= min.x || max.y <= min.y || max.z <= min.z)
            {
                continue;
            }

            float hitT          = 0.0f;
            float hitTMax       = maxDistance;
            bool valid          = true;
            Direction *hitFace  = nullptr;
            Direction *exitFace = nullptr;

            for (int axis = 0; axis < 3 && valid; axis++)
            {
                double axisOrigin    = axis == 0 ? origin.x : (axis == 1 ? origin.y : origin.z);
                double axisDirection = axis == 0 ? normalizedDirection.x : (axis == 1 ? normalizedDirection.y : normalizedDirection.z);
                double axisMin       = axis == 0 ? min.x : (axis == 1 ? min.y : min.z);
                double axisMax       = axis == 0 ? max.x : (axis == 1 ? max.y : max.z);

                Direction *axisNearFace = axis == 0 ? Direction::west : (axis == 1 ? Direction::down : Direction::north);
                Direction *axisFarFace  = axis == 0 ? Direction::east : (axis == 1 ? Direction::up : Direction::south);

                if (axisDirection != 0.0)
                {
                    float inverseAxisDirection = (float) (1.0 / axisDirection);
                    float entryT               = (float) ((axisMin - axisOrigin) * inverseAxisDirection);
                    float exitT                = (float) ((axisMax - axisOrigin) * inverseAxisDirection);
                    if (entryT > exitT)
                    {
                        std::swap(entryT, exitT);
                        std::swap(axisNearFace, axisFarFace);
                    }

                    if (entryT > hitT)
                    {
                        hitT    = entryT;
                        hitFace = axisNearFace;
                    }

                    if (exitT < hitTMax)
                    {
                        hitTMax  = exitT;
                        exitFace = axisFarFace;
                    }

                    if (hitTMax < hitT)
                    {
                        valid = false;
                    }
                }
                else if (axisOrigin < axisMin || axisOrigin > axisMax)
                {
                    valid = false;
                }
            }

            float boxHitT = hitT >= 0.0f ? hitT : hitTMax;
            if (!valid || boxHitT < 0.0f || boxHitT > maxDistance || boxHitT >= finalHitT)
            {
                continue;
            }

            if (block->GetRenderShape() == Block::eRenderShape_BOXES && block->HasInteractionAABB() && boxIndex + 1 == boxes.size())
            {
                if (finalFace && hitFace)
                {
                    finalFace = hitFace;
                }

                continue;
            }

            finalHitT = boxHitT;
            finalFace = hitFace ? hitFace : (face ? face : exitFace);
        }

        if (!finalFace)
        {
            continue;
        }

        bestBlockT   = finalHitT;
        bestBlockPos = pos;
        bestFace     = finalFace;
        hasBlock     = true;
        break;
    }

    if (hasEntity && (!hasBlock || bestEntityT <= bestBlockT))
    {
        glm::dvec3 hitPos = origin + normalizedDirection * (double) bestEntityT;
        result->SetEntity(hitPos, bestEntityId, bestEntityType, bestEntityT);
        return result;
    }

    if (hasBlock)
    {
        glm::dvec3 hitPos = origin + normalizedDirection * (double) bestBlockT;
        result->SetBlock(hitPos, bestBlockPos, bestFace, bestBlockT);
        return result;
    }

    return result;
}

void Level::SetRenderDistance(int distance)
{
    m_renderDistance = distance;
}

int Level::GetRenderDistance() const
{
    return m_renderDistance;
}

void Level::SetSimulationDistance(int distance)
{
    m_simulationDistance = distance;
}

int Level::GetSimulationDistance() const
{
    return m_simulationDistance;
}

void Level::SetPlayerSimulationCenters(const std::vector<ChunkPos> &centers)
{
    m_playerSimulationCenters = centers;
}

bool Level::IsInSimulationDistance(const glm::dvec3 &position) const
{
    if (m_simulationCenters.empty())
    {
        return true;
    }

    int chunkX = (int) floor(position.x) >> 4;
    int chunkZ = (int) floor(position.z) >> 4;
    for (size_t i = 0; i < m_simulationCenters.size(); i++)
    {
        int deltaX = chunkX - m_simulationCenters[i].x;
        int deltaZ = chunkZ - m_simulationCenters[i].z;
        if (deltaX * deltaX + deltaZ * deltaZ <= m_simulationDistance * m_simulationDistance)
        {
            return true;
        }
    }

    return false;
}

void Level::SetSpawnAreaRadius(int radius)
{
    m_spawnAreaRadius = std::clamp(radius, 0, s_maxSpawnAreaRadius);
}

void Level::SetWorldSpawn(const BlockPos &pos)
{
    m_worldSpawn    = pos;
    m_hasWorldSpawn = true;
}

const BlockPos &Level::GetWorldSpawn() const
{
    return m_worldSpawn;
}

bool Level::HasWorldSpawn() const
{
    return m_hasWorldSpawn;
}

void Level::EnsureChunkLoaded(const ChunkPos &pos)
{
    if (HasChunk(pos) || !m_chunkManager)
    {
        return;
    }

    std::unique_ptr<Chunk> chunk = m_chunkManager->ProduceChunkNow(pos);
    if (chunk)
    {
        InsertChunk(pos, std::move(chunk));
    }
}

glm::dvec3 Level::FindPlayerSpawn(const BlockPos &center)
{
    if (m_genSettings.worldType == eWorldType_Caves)
    {
        int centerX = Mth::FloorDiv(center.x, Chunk::s_sizeX);
        int centerZ = Mth::FloorDiv(center.z, Chunk::s_sizeZ);
        for (int radius = 0; radius <= 3; radius++)
        {
            for (int dx = -radius; dx <= radius; dx++)
            {
                for (int dz = -radius; dz <= radius; dz++)
                {
                    if (std::max(abs(dx), abs(dz)) != radius)
                    {
                        continue;
                    }

                    ChunkPos chunkPos(centerX + dx, centerZ + dz);
                    EnsureChunkLoaded(chunkPos);
                    // We sample odd columns from 1 to 13 so the lava check around a column stays inside this chunk
                    for (int column = 0; column < 49; column++)
                    {
                        int x = chunkPos.x * Chunk::s_sizeX + (column % 7) * 2 + 1;
                        int z = chunkPos.z * Chunk::s_sizeZ + (column / 7) * 2 + 1;
                        for (int y = TerrainGenerator::s_caveCeiling - 8; y > 0; y--)
                        {
                            Block *ground = Block::ById(GetBlockId(BlockPos(x, y, z)));
                            Block *feet   = Block::ById(GetBlockId(BlockPos(x, y + 1, z)));
                            Block *head   = Block::ById(GetBlockId(BlockPos(x, y + 2, z)));
                            if (!ground->IsSolid() || ground->IsTransparent() || feet->IsSolid() || feet->IsLiquid() || head->IsSolid() ||
                                head->IsLiquid())
                            {
                                continue;
                            }

                            bool lava = false;
                            for (int ly = y; ly <= y + 2 && !lava; ly++)
                            {
                                for (int lx = x - 1; lx <= x + 1 && !lava; lx++)
                                {
                                    for (int lz = z - 1; lz <= z + 1 && !lava; lz++)
                                    {
                                        lava = Block::ById(GetBlockId(BlockPos(lx, ly, lz)))->IsLava();
                                    }
                                }
                            }

                            if (!lava)
                            {
                                return glm::dvec3(x + 0.5, y + 1.0, z + 0.5);
                            }
                        }
                    }
                }
            }
        }

        Block *air = Block::ByName("air");
        SetBlock(BlockPos(center.x, center.y - 1, center.z), Block::ByName("stone"));
        SetBlock(center, air);
        SetBlock(BlockPos(center.x, center.y + 1, center.z), air);
        return glm::dvec3(center.x + 0.5, center.y, center.z + 0.5);
    }

    const int spawnFuzz = 10;

    for (int attempt = 0; attempt < 16; attempt++)
    {
        int x = center.x + (attempt == 15 ? 0 : m_random.NextInt(-spawnFuzz, spawnFuzz));
        int z = center.z + (attempt == 15 ? 0 : m_random.NextInt(-spawnFuzz, spawnFuzz));
        EnsureChunkLoaded(ChunkPos(Mth::FloorDiv(x, Chunk::s_sizeX), Mth::FloorDiv(z, Chunk::s_sizeZ)));

        for (int y = Chunk::s_sizeY - 3; y > 0; y--)
        {
            Block *ground = Block::ById(GetBlockId(BlockPos(x, y, z)));
            if (!ground->IsSolid() && !ground->IsLiquid())
            {
                continue;
            }

            Block *feet = Block::ById(GetBlockId(BlockPos(x, y + 1, z)));
            Block *head = Block::ById(GetBlockId(BlockPos(x, y + 2, z)));
            if (ground->IsSolid() && !ground->IsTransparent() && !feet->IsSolid() && !feet->IsLiquid() && !head->IsSolid() && !head->IsLiquid())
            {
                return glm::dvec3(x + 0.5, y + 1.0, z + 0.5);
            }

            break;
        }
    }

    return glm::dvec3(center.x + 0.5, center.y, center.z + 0.5);
}

int Level::GetSpawnAreaRadius() const
{
    return m_spawnAreaRadius;
}

Random &Level::GetRandom()
{
    return m_random;
}

void Level::SetSeed(uint32_t seed)
{
    m_seed = seed;
}

uint32_t Level::GetSeed() const
{
    return m_seed;
}

void Level::SetGenSettings(const WorldGenSettings &settings)
{
    m_genSettings = settings;
}

const WorldGenSettings &Level::GetGenSettings() const
{
    return m_genSettings;
}

uint8_t Level::GetLightLevel(const BlockPos &pos) const
{
    uint8_t sky   = GetSkyLightLevel(pos);
    uint8_t block = GetBlockLightLevel(pos);
    return std::max(sky, block);
}

// This uses the stepped sky darkening instead of the render curve because growth checks are tuned to its thresholds
int Level::GetSkyDarken() const
{
    float daylight = 1.0f - std::clamp(1.0f - ((float) std::cos(GetCelestialAngle() * std::numbers::pi * 2.0) * 2.0f + 0.5f), 0.0f, 1.0f);
    daylight *= 1.0f - GetRainLevel(1.0f) * 5.0f / 16.0f;
    daylight *= 1.0f - GetThunderLevel(1.0f) * 5.0f / 16.0f;
    return (int) ((1.0f - daylight) * 11.0f);
}

int Level::GetRawBrightness(const BlockPos &pos) const
{
    return std::max(GetSkyLightLevel(pos) - GetSkyDarken(), (int) GetBlockLightLevel(pos));
}

float Level::GetBrightness(const BlockPos &pos) const
{
    float darkness = 1.0f - GetRawBrightness(pos) / 15.0f;
    return (1.0f - darkness) / (darkness * 3.0f + 1.0f);
}

uint8_t Level::GetSkyLightLevel(const BlockPos &pos) const
{
    if (pos.y >= Chunk::s_sizeY)
    {
        return 15;
    }

    return LightEngine::GetSkyLight((Level *) this, pos);
}

uint8_t Level::GetBlockLightLevel(const BlockPos &pos) const
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    LightEngine::GetBlockLight((Level *) this, pos, &r, &g, &b);
    return std::max(r, std::max(g, b));
}

void Level::QueueLightUpdate(const BlockPos &pos)
{
    m_lightUpdates.push_back(pos);
}

void Level::QueueLightUpdateIfChanged(const BlockPos &pos, Block *oldBlock, Block *block)
{
    bool oldBlocksLight = oldBlock ? oldBlock->BlocksLight() : false;
    bool newBlocksLight = block ? block->BlocksLight() : false;
    uint8_t oldEmission = oldBlock ? oldBlock->GetLightEmission() : 0;
    uint8_t newEmission = block ? block->GetLightEmission() : 0;
    int oldOpacity      = oldBlock ? oldBlock->GetLightOpacity() : 0;
    int newOpacity      = block ? block->GetLightOpacity() : 0;
    if (oldBlocksLight != newBlocksLight || oldEmission != newEmission || oldOpacity != newOpacity)
    {
        QueueLightUpdate(pos);
    }
}

size_t Level::GetQueuedLightUpdateCount() const
{
    return m_lightUpdates.size();
}

bool Level::PollLightUpdate(BlockPos *outPos)
{
    if (m_lightUpdates.empty())
    {
        return false;
    }

    *outPos = m_lightUpdates.front();
    m_lightUpdates.pop_front();
    return true;
}

bool Level::PollSkyLightSeamUpdate(ChunkPos *outPos, const ChunkPos &center)
{
    if (m_skyLightSeamUpdates.empty())
    {
        return false;
    }

    size_t nearest                 = FindNearest(m_skyLightSeamUpdates, center);
    *outPos                        = m_skyLightSeamUpdates[nearest];
    m_skyLightSeamUpdates[nearest] = m_skyLightSeamUpdates.back();
    m_skyLightSeamUpdates.pop_back();
    m_skyLightSeamUpdatesSet.erase(*outPos);
    return true;
}

bool Level::IsSkyLightSeamQueued(const ChunkPos &pos) const
{
    return m_skyLightSeamUpdatesSet.find(pos) != m_skyLightSeamUpdatesSet.end();
}

bool Level::IsLiquidAt(const glm::dvec3 &position) const
{
    return IsLiquidAt(position, false);
}

bool Level::IsLavaAt(const glm::dvec3 &position) const
{
    return IsLiquidAt(position, true);
}

bool Level::IsLiquidAt(const glm::dvec3 &position, bool lava) const
{
    BlockPos pos((int) floor(position.x), (int) floor(position.y), (int) floor(position.z));
    Block *block = Block::ById(GetBlockId(pos));
    if (!block || !block->IsLiquid() || block->IsLava() != lava)
    {
        return false;
    }

    Block *above = Block::ById(GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)));
    return (above && above->IsLiquid()) || position.y <= (double) pos.y + block->GetLiquidHeight(GetBlockData(pos));
}

WorldProvider &Level::GetWorldProvider()
{
    return m_worldProvider;
}

const WorldProvider &Level::GetWorldProvider() const
{
    return m_worldProvider;
}

void Level::SetWorldTime(int64_t ticks)
{
    m_worldProvider.SetWorldTime(ticks);
}

float Level::GetCelestialAngle() const
{
    return m_worldProvider.GetCelestialAngle();
}

uint8_t Level::GetSkyLightClamp() const
{
    return m_worldProvider.GetSkyLightClamp();
}

void Level::SetWeather(bool raining, int rainTime, bool thundering, int thunderTime)
{
    m_raining     = raining;
    m_rainTime    = rainTime;
    m_thundering  = thundering;
    m_thunderTime = thunderTime;
    OnWeatherChanged();
}

void Level::SetWeatherLevels(float rainLevel, float thunderLevel)
{
    m_rainLevel       = rainLevel;
    m_oldRainLevel    = rainLevel;
    m_thunderLevel    = thunderLevel;
    m_oldThunderLevel = thunderLevel;
}

int64_t Level::GetGameTime() const
{
    return m_gameTime;
}

bool Level::IsRaining() const
{
    return m_raining;
}

bool Level::IsThundering() const
{
    return m_thundering;
}

int Level::GetRainTime() const
{
    return m_rainTime;
}

int Level::GetThunderTime() const
{
    return m_thunderTime;
}

float Level::GetRainLevel(float partialTicks) const
{
    return m_oldRainLevel + (m_rainLevel - m_oldRainLevel) * partialTicks;
}

float Level::GetThunderLevel(float partialTicks) const
{
    return (m_oldThunderLevel + (m_thunderLevel - m_oldThunderLevel) * partialTicks) * GetRainLevel(partialTicks);
}

bool Level::IsRainingAt(const BlockPos &pos) const
{
    if (GetRainLevel(1.0f) <= 0.2f || pos.y < GetRainHeight(pos.x, pos.z))
    {
        return false;
    }

    const Chunk *chunk = GetChunk(ChunkPos(pos.x >> 4, pos.z >> 4));
    if (!chunk)
    {
        return false;
    }

    Biome *biome = chunk->GetSurfaceBiome(pos.x & 15, pos.z & 15);
    return biome->HasRain() && !biome->IsSnowyAt(pos.y);
}

bool Level::IsSnowingAt(const BlockPos &pos) const
{
    if (GetRainLevel(1.0f) <= 0.2f || pos.y < GetRainHeight(pos.x, pos.z))
    {
        return false;
    }

    const Chunk *chunk = GetChunk(ChunkPos(pos.x >> 4, pos.z >> 4));
    return chunk && chunk->GetSurfaceBiome(pos.x & 15, pos.z & 15)->IsSnowyAt(pos.y);
}

int Level::GetRainHeight(int x, int z) const
{
    const Chunk *chunk = GetChunk(ChunkPos(x >> 4, z >> 4));
    if (!chunk)
    {
        return 0;
    }

    for (int section = Chunk::s_sectionCount - 1; section >= 0; section--)
    {
        if (chunk->IsSectionEmpty(section))
        {
            continue;
        }

        for (int y = section * ChunkSection::s_size + ChunkSection::s_size - 1; y >= section * ChunkSection::s_size; y--)
        {
            Block *block = Block::ById(chunk->GetBlockId(x & 15, y, z & 15));
            if (block->IsSolid() || block->IsLiquid())
            {
                return y + 1;
            }
        }
    }

    return 0;
}

void Level::OnWeatherChanged()
{}

void Level::OnEntityHurt(Entity *entity)
{
    (void) entity;
}

void Level::OnMobDataChanged(Mob *mob)
{
    (void) mob;
}

void Level::BroadcastEntityEvent(Entity *entity, uint8_t event)
{
    (void) entity;
    (void) event;
}

void Level::OnPassengersChanged(Entity *vehicle)
{
    (void) vehicle;
}

void Level::OnBoatDataChanged(Boat *boat)
{
    (void) boat;
}

void Level::AddLightningFlash(const glm::dvec3 &position, float strength)
{
    m_lightningFlashAge      = 0;
    m_lightningFlashStrength = strength;
    m_lightningFlashPosition = position;
}

float Level::GetLightningFlash(float partialTicks) const
{
    const float envelope[] = {1.0f, 0.35f, 0.9f, 0.5f, 0.25f, 0.1f, 0.0f};

    float time = (float) m_lightningFlashAge + partialTicks;
    int index  = (int) time;
    if (m_lightningFlashAge < 0 || index >= (int) std::size(envelope) - 1)
    {
        return 0.0f;
    }

    return std::lerp(envelope[index], envelope[index + 1], time - (float) index) * m_lightningFlashStrength;
}

const glm::dvec3 &Level::GetLightningFlashPosition() const
{
    return m_lightningFlashPosition;
}

void Level::QueueChunkSkyLightSeamUpdate(const ChunkPos &pos)
{
    if (m_skyLightSeamUpdatesSet.insert(pos).second)
    {
        m_skyLightSeamUpdates.push_back(pos);
    }
}
