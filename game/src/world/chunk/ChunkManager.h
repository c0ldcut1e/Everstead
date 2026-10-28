#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "../../threading/ThreadPool.h"
#include "../../utils/heap/BinaryHeap.h"
#include "../Level.h"
#include "ChunkPos.h"

class ChunkManager
{
public:
    explicit ChunkManager(Level *level);
    ~ChunkManager();

    void Start();
    void Stop();
    void SetWorldName(const std::string &worldName);
    void Update(const glm::dvec3 &playerPosition);
    void Update(const std::vector<glm::dvec3> &playerPositions);

    void DrainFinished(std::deque<std::pair<ChunkPos, std::unique_ptr<Chunk>>> *out, int max);
    size_t GetPendingCount() const;
    size_t GetActiveCount() const;
    size_t GetMaxActiveCount() const;
    size_t GetFinishedCount() const;
    size_t GetThreadCount() const;

    bool TakeGenerationFailure(ChunkPos *outPos);
    std::unique_ptr<Chunk> ProduceChunkNow(const ChunkPos &pos);

private:
    struct GenerationTask
    {
        ChunkPos pos;
        int dist2;
        uint32_t epoch;
    };

    struct TaskCompare
    {
        bool operator()(const GenerationTask &a, const GenerationTask &b) const
        {
            if (a.dist2 != b.dist2)
            {
                return a.dist2 > b.dist2;
            }

            if (a.pos.x != b.pos.x)
            {
                return a.pos.x > b.pos.x;
            }

            return a.pos.z > b.pos.z;
        }
    };

    void GenerateChunk(const ChunkPos &pos);
    void RebuildChunkSkyLight(Chunk *chunk) const;
    void QueueChunkGeneration(const ChunkPos &pos);
    bool IsChunkInRenderDistance(const ChunkPos &pos, const ChunkPos &center) const;
    bool IsChunkInAnyRenderDistance(const ChunkPos &pos) const;
    int CalculatePriority(const ChunkPos &pos, const ChunkPos &center) const;

    size_t RebuildPending(const std::vector<ChunkPos> &centers, const std::unordered_set<ChunkPos, ChunkPosHash> &known);
    void CollectKnownChunks(std::unordered_set<ChunkPos, ChunkPosHash> *out);
    void RecordGenerationFailure(const ChunkPos &pos);
    void DispatchPending();

    bool ShouldStartTask(const ChunkPos &pos, uint32_t taskEpoch) const;
    bool ShouldKeepResult(const ChunkPos &pos) const;
    bool IsFinishedBacklogFull() const;

    static const size_t s_maxFinishedBacklog = 512;

    static std::weak_ptr<ThreadPool> s_sharedPool;

    Level *m_level;

    std::shared_ptr<ThreadPool> m_pool;
    std::atomic<bool> m_running;

    BinaryHeap<GenerationTask, TaskCompare> m_pending;
    std::unordered_set<ChunkPos, ChunkPosHash> m_pendingSet;
    mutable std::mutex m_pendingMutex;

    std::unordered_set<ChunkPos, ChunkPosHash> m_activeSet;
    std::mutex m_activeMutex;

    std::atomic<int> m_active;
    std::mutex m_idleMutex;
    std::condition_variable m_idleCv;
    int m_maxActive;

    std::deque<std::pair<ChunkPos, std::unique_ptr<Chunk>>> m_finished;
    std::unordered_set<ChunkPos, ChunkPosHash> m_finishedSet;
    mutable std::mutex m_finishedMutex;

    ChunkPos m_lastPlayerChunk;
    std::vector<ChunkPos> m_lastPlayerChunks;
    mutable std::mutex m_centersMutex;

    std::atomic<int> m_centerX;
    std::atomic<int> m_centerZ;
    std::atomic<uint32_t> m_epoch;

    std::atomic<int> m_renderDistance;
    bool m_pendingEmptyForCenter;

    std::unordered_set<ChunkPos, ChunkPosHash> m_knownScratch;
    std::unordered_set<ChunkPos, ChunkPosHash> m_activeScratch;
    std::vector<GenerationTask> m_taskScratch;

    bool m_hasGenerationFailure;
    ChunkPos m_generationFailurePos;
    std::mutex m_generationFailureMutex;

    std::string m_worldName;
    std::mutex m_worldNameMutex;
};
