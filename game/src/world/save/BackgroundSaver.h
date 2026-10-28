#pragma once

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../chunk/ChunkPos.h"

class EversteadServer;
class Level;
class SaveService;

class BackgroundSaver
{
public:
    BackgroundSaver();

    void Clear();
    void Begin(EversteadServer *server, SaveService *saveService, const std::string &worldName);
    void Drain(SaveService *saveService, const std::string &worldName);
    bool HasPendingChunks() const;

private:
    static const int s_maxChunksPerTick = 16;

    std::vector<std::pair<Level *, ChunkPos>> m_pendingChunks;
    std::unordered_set<ChunkPos, ChunkPosHash> m_scratch;
};
