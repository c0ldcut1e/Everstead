#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "ChunkPos.h"

#include <glm/vec3.hpp>

class Level;
class SaveService;

class ChunkUnloader
{
public:
    ChunkUnloader();

    void Reset();
    void UnloadClientChunks(Level *level, const glm::dvec3 &playerPosition, int unloadDistance);
    void UnloadServerChunks(Level *level, const std::vector<glm::dvec3> &playerPositions, int unloadDistance, SaveService *saveService,
                            const std::string &worldName);

private:
    static inline const int s_unloadBudgetPerFrame = 2;
    static const int s_unloadBacklogFrames         = 60;

    static ChunkPos ToChunkPos(const glm::dvec3 &position);
    static bool IsNearCenters(const ChunkPos &pos, const std::vector<ChunkPos> &centers, int maxDistanceSquared);
    static int GetBudget(size_t candidateCount);

    std::unordered_set<ChunkPos, ChunkPosHash> m_clientScratch;
    std::unordered_set<ChunkPos, ChunkPosHash> m_serverScratch;
    std::vector<ChunkPos> m_clientCandidates;
    std::vector<ChunkPos> m_serverCandidates;
    std::vector<ChunkPos> m_lastClientCenters;
    std::vector<ChunkPos> m_lastServerCenters;
    int m_lastClientDistance;
    int m_lastServerDistance;
};
