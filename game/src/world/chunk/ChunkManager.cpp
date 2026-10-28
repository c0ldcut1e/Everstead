#include "ChunkManager.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <new>

#include "../../core/EversteadServer.h"
#include "../../core/Logger.h"
#include "../../core/MemoryMonitor.h"
#include "../../threading/ThreadStorage.h"
#include "../../utils/math/Mth.h"
#include "../LevelRenderer.h"
#include "../block/Block.h"
#include "../block/BlockRenderer.h"
#include "../generation/TerrainGenerator.h"
#include "../lighting/LightEngine.h"
#include "../lod/LodGenerator.h"
#include "../save/WorldSave.h"

std::weak_ptr<ThreadPool> ChunkManager::s_sharedPool;

ChunkManager::ChunkManager(Level *level)
{
    m_level                 = level;
    m_running               = false;
    m_active                = 0;
    m_maxActive             = 0;
    m_lastPlayerChunk       = ChunkPos(INT32_MAX, INT32_MAX);
    m_lastPlayerChunks      = {};
    m_centerX               = 0;
    m_centerZ               = 0;
    m_epoch                 = 0;
    m_renderDistance        = 0;
    m_pendingEmptyForCenter = false;
    m_hasGenerationFailure  = false;
    m_generationFailurePos  = ChunkPos(0, 0);
}

ChunkManager::~ChunkManager()
{
    Stop();
}

void ChunkManager::Start()
{
    if (m_running.load())
    {
        return;
    }

    m_running.store(true);

    int threadCount = EversteadServer::GetInstance()->settings.GetChunkWorkerThreadsPerPool();
    Logger::LogInfo("Starting chunk generation with %d threads", threadCount);

    // Every dimension generates on one pool, so a second dimension doesn't add threads fighting over the same cores
    m_pool = s_sharedPool.lock();
    if (!m_pool)
    {
        m_pool       = std::make_shared<ThreadPool>((size_t) threadCount);
        s_sharedPool = m_pool;
    }

    m_maxActive = std::max(1, (int) m_pool->GetThreadCount() * EversteadServer::GetInstance()->settings.chunkQueuePerThread);

    if (m_level)
    {
        m_renderDistance.store(m_level->GetRenderDistance());
    }
}

void ChunkManager::Stop()
{
    if (!m_running.load())
    {
        return;
    }

    m_running.store(false);

    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);

        while (!m_pending.Empty())
        {
            m_pending.Pop();
        }

        m_pendingSet.clear();
    }

    if (m_pool)
    {
        std::unique_lock<std::mutex> lock(m_idleMutex);
        m_idleCv.wait(lock,
                      [this]
                      {
                          return m_active.load() == 0;
                      });
        lock.unlock();
        m_pool.reset();
    }

    {
        std::lock_guard<std::mutex> lock(m_activeMutex);

        m_activeSet.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_finishedMutex);

        m_finished.clear();
        m_finishedSet.clear();
    }

    m_active.store(0);

    Logger::LogInfo("Chunk generation stopped");
}

bool ChunkManager::ShouldStartTask(const ChunkPos &pos, uint32_t taskEpoch) const
{
    if (!m_running.load())
    {
        return false;
    }

    if (taskEpoch != m_epoch.load())
    {
        return false;
    }

    if (!IsChunkInAnyRenderDistance(pos))
    {
        return false;
    }

    return true;
}

bool ChunkManager::ShouldKeepResult(const ChunkPos &pos) const
{
    if (!m_running.load())
    {
        return false;
    }

    if (!IsChunkInAnyRenderDistance(pos))
    {
        return false;
    }

    return true;
}

size_t ChunkManager::RebuildPending(const std::vector<ChunkPos> &centers, const std::unordered_set<ChunkPos, ChunkPosHash> &known)
{
    uint32_t epoch     = m_epoch.load();
    int renderDistance = m_renderDistance.load();
    int maxD2          = renderDistance * renderDistance;

    {
        std::lock_guard<std::mutex> lock(m_activeMutex);
        m_activeScratch = m_activeSet;
    }

    const std::unordered_set<ChunkPos, ChunkPosHash> &activeSnapshot = m_activeScratch;

    std::vector<GenerationTask> &tasks = m_taskScratch;
    tasks.clear();
    tasks.reserve((size_t) ((renderDistance * 2 + 1) * (renderDistance * 2 + 1) * (int) centers.size()));

    for (const ChunkPos &center : centers)
    {
        for (int dz = -renderDistance; dz <= renderDistance; dz++)
        {
            for (int dx = -renderDistance; dx <= renderDistance; dx++)
            {
                int d2 = dx * dx + dz * dz;
                if (d2 > maxD2)
                {
                    continue;
                }

                ChunkPos pos{center.x + dx, center.z + dz};
                if (known.count(pos) || activeSnapshot.count(pos))
                {
                    continue;
                }

                tasks.push_back({pos, d2, epoch});
            }
        }
    }

    std::lock_guard<std::mutex> lock(m_pendingMutex);
    while (!m_pending.Empty())
    {
        m_pending.Pop();
    }

    m_pendingSet.clear();

    size_t queuedCount = 0;
    for (const GenerationTask &task : tasks)
    {
        if (m_pendingSet.insert(task.pos).second)
        {
            m_pending.Push(task);
            queuedCount++;
        }
    }

    return queuedCount;
}

void ChunkManager::CollectKnownChunks(std::unordered_set<ChunkPos, ChunkPosHash> *out)
{
    m_level->CollectLoadedChunkPositions(out);

    std::lock_guard<std::mutex> lock(m_finishedMutex);
    for (const ChunkPos &pos : m_finishedSet)
    {
        out->insert(pos);
    }
}

void ChunkManager::Update(const glm::dvec3 &playerPosition)
{
    std::vector<glm::dvec3> playerPositions;
    playerPositions.push_back(playerPosition);
    Update(playerPositions);
}

void ChunkManager::Update(const std::vector<glm::dvec3> &playerPositions)
{
    std::vector<ChunkPos> playerChunks;
    playerChunks.reserve(playerPositions.size());
    for (const glm::dvec3 &playerPosition : playerPositions)
    {
        int cx = Mth::FloorDiv((int) floor(playerPosition.x), Chunk::s_sizeX);
        int cz = Mth::FloorDiv((int) floor(playerPosition.z), Chunk::s_sizeZ);
        ChunkPos playerChunk{cx, cz};
        if (std::find(playerChunks.begin(), playerChunks.end(), playerChunk) == playerChunks.end())
        {
            playerChunks.push_back(playerChunk);
        }
    }

    // The unloader drops every chunk of a level nobody is in, so we forget the centers to rebuild when someone returns
    if (playerChunks.empty())
    {
        std::lock_guard<std::mutex> lock(m_centersMutex);
        m_lastPlayerChunks.clear();
        return;
    }

    bool renderDistanceChanged = false;
    if (m_level)
    {
        renderDistanceChanged = m_renderDistance.exchange(m_level->GetRenderDistance()) != m_level->GetRenderDistance();
    }

    if (!m_pool || !m_running.load())
    {
        m_lastPlayerChunk = playerChunks[0];
        {
            std::lock_guard<std::mutex> lock(m_centersMutex);
            m_lastPlayerChunks = playerChunks;
        }

        return;
    }

    bool shouldRebuildPending = false;
    {
        std::lock_guard<std::mutex> lock(m_centersMutex);
        shouldRebuildPending = renderDistanceChanged || playerChunks != m_lastPlayerChunks;
    }

    if (!shouldRebuildPending && !m_pendingEmptyForCenter && m_active.load() == 0)
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        shouldRebuildPending = m_pending.Empty();
    }

    if (shouldRebuildPending)
    {
        m_lastPlayerChunk       = playerChunks[0];
        m_pendingEmptyForCenter = false;

        m_centerX.store(playerChunks[0].x);
        m_centerZ.store(playerChunks[0].z);
        m_epoch.fetch_add(1);

        {
            std::lock_guard<std::mutex> lock(m_centersMutex);
            m_lastPlayerChunks = playerChunks;
        }

        CollectKnownChunks(&m_knownScratch);

        size_t queuedCount      = RebuildPending(playerChunks, m_knownScratch);
        m_pendingEmptyForCenter = queuedCount == 0;
    }

    DispatchPending();
}

void ChunkManager::RecordGenerationFailure(const ChunkPos &pos)
{
    std::lock_guard<std::mutex> lock(m_generationFailureMutex);
    m_hasGenerationFailure = true;
    m_generationFailurePos = pos;
}

bool ChunkManager::TakeGenerationFailure(ChunkPos *outPos)
{
    std::lock_guard<std::mutex> lock(m_generationFailureMutex);
    if (!m_hasGenerationFailure)
    {
        return false;
    }

    m_hasGenerationFailure = false;
    if (outPos)
    {
        *outPos = m_generationFailurePos;
    }

    return true;
}

void ChunkManager::DispatchPending()
{
    if (!m_pool)
    {
        return;
    }

    if (!m_running.load())
    {
        return;
    }

    if (IsFinishedBacklogFull())
    {
        return;
    }

    if (MemoryMonitor::GetInstance()->IsUnderPressure())
    {
        ChunkPos blockedPos;
        bool hasBlockedPos = false;

        {
            std::lock_guard<std::mutex> lock(m_pendingMutex);
            if (!m_pending.Empty())
            {
                blockedPos    = m_pending.Top().pos;
                hasBlockedPos = true;
            }
        }

        if (hasBlockedPos)
        {
            RecordGenerationFailure(blockedPos);
        }

        return;
    }

    while (m_active.load() < m_maxActive && !IsFinishedBacklogFull())
    {
        GenerationTask task;

        {
            std::lock_guard<std::mutex> lock(m_pendingMutex);

            if (m_pending.Empty())
            {
                break;
            }

            task = m_pending.Top();
            m_pending.Pop();
            m_pendingSet.erase(task.pos);
        }

        if (!ShouldStartTask(task.pos, task.epoch))
        {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_activeMutex);

            if (m_activeSet.find(task.pos) != m_activeSet.end())
            {
                continue;
            }

            m_activeSet.insert(task.pos);
        }

        m_active.fetch_add(1);

        m_pool->DetachTask(
                [this, pos = task.pos]
                {
                    ThreadStorage::UseDefaultThreadStorage();
                    try
                    {
                        // We skip the epoch check because pending rebuilds skip active chunks and a dropped task would never be queued again
                        if (m_running.load() && IsChunkInAnyRenderDistance(pos))
                        {
                            GenerateChunk(pos);
                        }
                    }
                    catch (const std::exception &exception)
                    {
                        Logger::LogError("Chunk generation failed at %d,%d: %s", pos.x, pos.z, exception.what());
                        RecordGenerationFailure(pos);
                    }
                    catch (...)
                    {
                        Logger::LogError("Chunk generation failed at %d,%d", pos.x, pos.z);
                        RecordGenerationFailure(pos);
                    }

                    {
                        std::lock_guard<std::mutex> lock(m_activeMutex);

                        m_activeSet.erase(pos);
                    }

                    std::lock_guard<std::mutex> lock(m_idleMutex);
                    m_active.fetch_sub(1);
                    m_idleCv.notify_all();
                });
    }
}

void ChunkManager::DrainFinished(std::deque<std::pair<ChunkPos, std::unique_ptr<Chunk>>> *out, int max)
{
    std::lock_guard<std::mutex> lock(m_finishedMutex);

    std::vector<ChunkPos> centers;
    {
        std::lock_guard<std::mutex> centersLock(m_centersMutex);
        centers = m_lastPlayerChunks;
    }

    if (centers.empty())
    {
        centers.push_back(ChunkPos(m_centerX.load(), m_centerZ.load()));
    }

    int count = 0;
    while (!m_finished.empty() && count < max)
    {
        size_t nearest   = 0;
        int64_t nearest2 = INT64_MAX;
        for (size_t i = 0; i < m_finished.size(); i++)
        {
            // Every player gets their closest chunks first, not just the host
            for (const ChunkPos &center : centers)
            {
                int64_t dx = (int64_t) m_finished[i].first.x - center.x;
                int64_t dz = (int64_t) m_finished[i].first.z - center.z;
                if (dx * dx + dz * dz < nearest2)
                {
                    nearest  = i;
                    nearest2 = dx * dx + dz * dz;
                }
            }
        }

        out->emplace_back(std::move(m_finished[nearest]));
        m_finishedSet.erase(out->back().first);
        m_finished[nearest] = std::move(m_finished.back());
        m_finished.pop_back();
        count++;
    }
}

size_t ChunkManager::GetPendingCount() const
{
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    return m_pending.Size();
}

size_t ChunkManager::GetActiveCount() const
{
    return (size_t) m_active.load();
}

size_t ChunkManager::GetMaxActiveCount() const
{
    return (size_t) m_maxActive;
}

size_t ChunkManager::GetFinishedCount() const
{
    std::lock_guard<std::mutex> lock(m_finishedMutex);
    return m_finished.size();
}

size_t ChunkManager::GetThreadCount() const
{
    if (!m_pool)
    {
        return 0;
    }

    return m_pool->GetThreadCount();
}

bool ChunkManager::IsFinishedBacklogFull() const
{
    std::lock_guard<std::mutex> lock(m_finishedMutex);
    return m_finished.size() >= s_maxFinishedBacklog;
}

void ChunkManager::SetWorldName(const std::string &worldName)
{
    std::lock_guard<std::mutex> lock(m_worldNameMutex);
    m_worldName = worldName;
    if (m_level->GetDimensionId() == eDimensionId_Overworld)
    {
        LodGenerator::GetInstance()->GetStore()->Open(worldName, false);
    }
}

std::unique_ptr<Chunk> ChunkManager::ProduceChunkNow(const ChunkPos &pos)
{
    std::string worldName;
    {
        std::lock_guard<std::mutex> lock(m_worldNameMutex);
        worldName = m_worldName;
    }

    std::unique_ptr<Chunk> chunk;
    if (!worldName.empty())
    {
        chunk = WorldSave::LoadChunkFromRegion(worldName, m_level->GetDimensionId(), pos.x, pos.z);
    }

    bool generated = !chunk;
    if (generated)
    {
        chunk = std::make_unique<Chunk>(pos);
        thread_local std::unique_ptr<TerrainGenerator> generator;

        uint32_t worldSeed               = m_level->GetSeed();
        const WorldGenSettings &settings = m_level->GetGenSettings();
        if (!generator || generator->GetSeed() != worldSeed || !(generator->GetSettings() == settings))
        {
            generator = std::make_unique<TerrainGenerator>(worldSeed, settings);
        }

        generator->GenerateChunk(*chunk, pos);
        chunk->SetPendingMobSpawn(true);
    }

    // Saved chunks still reach the store through tile probes and saves, so we keep this off the chunk threads when LODs are disabled
    if (EversteadServer::GetInstance()->settings.lodEnabled && m_level->GetDimensionId() == eDimensionId_Overworld)
    {
        LodGenerator::GetInstance()->GetStore()->Ingest(worldName, chunk.get(), generated);
    }

    RebuildChunkSkyLight(chunk.get());
    LightEngine::BuildChunkBlockLight(chunk.get());
    chunk->CollectLiquidTicks();
    chunk->Compact();
    return chunk;
}

void ChunkManager::GenerateChunk(const ChunkPos &pos)
{
    std::unique_ptr<Chunk> chunk = ProduceChunkNow(pos);
    if (!ShouldKeepResult(pos))
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_finishedMutex);
    if (m_finishedSet.insert(pos).second)
    {
        m_finished.emplace_back(pos, std::move(chunk));
    }
}

void ChunkManager::RebuildChunkSkyLight(Chunk *chunk) const
{
    if (!chunk)
    {
        return;
    }

    int heightMap[Chunk::s_sizeX * Chunk::s_sizeZ];

    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            heightMap[x + Chunk::s_sizeX * z] = -1;
            bool blocked                      = false;
            for (int y = Chunk::s_sizeY - 1; y >= 0; y--)
            {
                if (blocked)
                {
                    chunk->SetSkyLight(x, y, z, 0);
                    continue;
                }

                Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                if (block->BlocksLight() || block->GetLightOpacity() > 0)
                {
                    blocked = true;
                    chunk->SetSkyLight(x, y, z, 0);
                    heightMap[x + Chunk::s_sizeX * z] = y;
                }
                else
                {
                    chunk->SetSkyLight(x, y, z, 15);
                }
            }
        }
    }

    static const int directions[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    std::vector<LightEngine::SkyLightNode> queueData;
    queueData.reserve(Chunk::s_sizeX * Chunk::s_sizeZ * 4);
    size_t queueHead = 0;

    static const int hOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            int topSolid = heightMap[x + Chunk::s_sizeX * z];

            for (int y = topSolid + 1; y < Chunk::s_sizeY; y++)
            {
                bool needsSeed = (y == topSolid + 1);

                for (int i = 0; i < 4 && !needsSeed; i++)
                {
                    int nx = x + hOffsets[i][0];
                    int nz = z + hOffsets[i][1];

                    if (nx < 0 || nx >= Chunk::s_sizeX || nz < 0 || nz >= Chunk::s_sizeZ)
                    {
                        continue;
                    }

                    if (heightMap[nx + Chunk::s_sizeX * nz] >= y)
                    {
                        needsSeed = true;
                    }
                }

                if (needsSeed)
                {
                    queueData.push_back({x, y, z, 15});
                }
            }
        }
    }

    while (queueHead < queueData.size())
    {
        LightEngine::SkyLightNode node = queueData[queueHead++];

        uint8_t currentLevel = chunk->GetSkyLight(node.x, node.y, node.z);
        if (currentLevel == 0)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];
            if (nx < 0 || nx >= Chunk::s_sizeX || ny < 0 || ny >= Chunk::s_sizeY || nz < 0 || nz >= Chunk::s_sizeZ)
            {
                continue;
            }

            Block *neighborBlock = Block::ById(chunk->GetBlockId(nx, ny, nz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t neighborLevel = chunk->GetSkyLight(nx, ny, nz);

            uint8_t newLevel;
            if (directions[i][1] == -1 && currentLevel == 15 && neighborBlock->GetLightOpacity() == 0)
            {
                newLevel = 15;
            }
            else
            {
                newLevel = LightEngine::Attenuate(currentLevel, neighborBlock);
            }

            if (newLevel > neighborLevel)
            {
                chunk->SetSkyLight(nx, ny, nz, newLevel);
                queueData.push_back({nx, ny, nz, newLevel});
            }
        }
    }
}

void ChunkManager::QueueChunkGeneration(const ChunkPos &pos)
{
    if (!m_pool)
    {
        return;
    }

    if (!m_running.load())
    {
        return;
    }

    ChunkPos center{m_centerX.load(), m_centerZ.load()};
    if (!IsChunkInRenderDistance(pos, center))
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_activeMutex);

        if (m_activeSet.find(pos) != m_activeSet.end())
        {
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);

        if (m_pendingSet.find(pos) != m_pendingSet.end())
        {
            return;
        }

        m_pendingSet.insert(pos);

        int dx = pos.x - center.x;
        int dz = pos.z - center.z;
        int d2 = dx * dx + dz * dz;

        m_pending.Push({pos, d2, m_epoch.load()});
    }

    DispatchPending();
}

bool ChunkManager::IsChunkInRenderDistance(const ChunkPos &pos, const ChunkPos &center) const
{
    int dx             = pos.x - center.x;
    int dz             = pos.z - center.z;
    int renderDistance = m_renderDistance.load();
    return (dx * dx + dz * dz) <= (renderDistance * renderDistance);
}

bool ChunkManager::IsChunkInAnyRenderDistance(const ChunkPos &pos) const
{
    std::lock_guard<std::mutex> lock(m_centersMutex);
    for (const ChunkPos &center : m_lastPlayerChunks)
    {
        if (IsChunkInRenderDistance(pos, center))
        {
            return true;
        }
    }

    ChunkPos center{m_centerX.load(), m_centerZ.load()};
    return IsChunkInRenderDistance(pos, center);
}

int ChunkManager::CalculatePriority(const ChunkPos &pos, const ChunkPos &center) const
{
    int dx = pos.x - center.x;
    int dz = pos.z - center.z;
    return -(dx * dx + dz * dz);
}
