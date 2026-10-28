#include "ChunkUnloader.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "../save/SaveService.h"
#include "../save/WorldSave.h"
#include "Chunk.h"

ChunkUnloader::ChunkUnloader()
{
    Reset();
}

void ChunkUnloader::Reset()
{
    m_clientScratch.clear();
    m_serverScratch.clear();
    m_clientCandidates.clear();
    m_serverCandidates.clear();
    m_lastClientCenters.clear();
    m_lastServerCenters.clear();
    m_lastClientDistance = -1;
    m_lastServerDistance = -1;
}

void ChunkUnloader::UnloadClientChunks(Level *level, const glm::dvec3 &playerPosition, int unloadDistance)
{
    int unloadDistanceSquared     = unloadDistance * unloadDistance;
    std::vector<ChunkPos> centers = {ToChunkPos(playerPosition)};
    if (centers != m_lastClientCenters || unloadDistance != m_lastClientDistance)
    {
        m_lastClientCenters  = centers;
        m_lastClientDistance = unloadDistance;

        level->CollectLoadedChunkPositions(&m_clientScratch);
        m_clientCandidates.clear();
        for (const ChunkPos &pos : m_clientScratch)
        {
            if (!IsNearCenters(pos, centers, unloadDistanceSquared))
            {
                m_clientCandidates.push_back(pos);
            }
        }
    }

    int budget = GetBudget(m_clientCandidates.size());
    while (!m_clientCandidates.empty() && budget > 0)
    {
        ChunkPos pos = m_clientCandidates.back();
        m_clientCandidates.pop_back();
        if (IsNearCenters(pos, centers, unloadDistanceSquared) || level->IsChunkMeshBusy(pos))
        {
            continue;
        }

        level->ReleaseChunkMeshes(pos);
        level->RemoveBlockEntities(pos);
        level->RemoveChunk(pos);
        budget--;
    }
}

void ChunkUnloader::UnloadServerChunks(Level *level, const std::vector<glm::dvec3> &playerPositions, int unloadDistance, SaveService *saveService,
                                       const std::string &worldName)
{
    int unloadDistanceSquared = unloadDistance * unloadDistance;
    std::vector<ChunkPos> centers;
    centers.reserve(playerPositions.size());
    for (size_t i = 0; i < playerPositions.size(); i++)
    {
        centers.push_back(ToChunkPos(playerPositions[i]));
    }

    if (centers != m_lastServerCenters || unloadDistance != m_lastServerDistance)
    {
        m_lastServerCenters  = centers;
        m_lastServerDistance = unloadDistance;

        level->CollectLoadedChunkPositions(&m_serverScratch);
        m_serverCandidates.clear();
        for (const ChunkPos &pos : m_serverScratch)
        {
            if (!IsNearCenters(pos, centers, unloadDistanceSquared))
            {
                m_serverCandidates.push_back(pos);
            }
        }
    }

    int budget = GetBudget(m_serverCandidates.size());
    while (!m_serverCandidates.empty() && budget > 0)
    {
        ChunkPos pos = m_serverCandidates.back();
        m_serverCandidates.pop_back();
        if (IsNearCenters(pos, centers, unloadDistanceSquared))
        {
            continue;
        }

        std::unique_ptr<Chunk> removed = level->RemoveChunk(pos);
        if (!removed)
        {
            continue;
        }

        budget--;
        level->StoreBlockTicks(removed.get(), true);
        level->StoreBlockEntities(removed.get(), true);
        if (removed->NeedsSave() || !removed->GetBlockTicks().empty() || !removed->GetBlockEntities().empty())
        {
            WorldSave::EnqueueChunkSave(saveService, worldName, level->GetDimensionId(), std::move(removed));
        }
    }
}

ChunkPos ChunkUnloader::ToChunkPos(const glm::dvec3 &position)
{
    return ChunkPos(Mth::FloorDiv((int) floor(position.x), Chunk::s_sizeX), Mth::FloorDiv((int) floor(position.z), Chunk::s_sizeZ));
}

bool ChunkUnloader::IsNearCenters(const ChunkPos &pos, const std::vector<ChunkPos> &centers, int maxDistanceSquared)
{
    for (size_t i = 0; i < centers.size(); i++)
    {
        int deltaX = pos.x - centers[i].x;
        int deltaZ = pos.z - centers[i].z;
        if (deltaX * deltaX + deltaZ * deltaZ <= maxDistanceSquared)
        {
            return true;
        }
    }

    return false;
}

int ChunkUnloader::GetBudget(size_t candidateCount)
{
    return std::max(s_unloadBudgetPerFrame, (int) candidateCount / s_unloadBacklogFrames);
}
