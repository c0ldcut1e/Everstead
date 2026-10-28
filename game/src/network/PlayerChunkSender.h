#pragma once

#include <cstddef>
#include <unordered_set>
#include <vector>

#include "../world/chunk/ChunkPos.h"

class ServerPlayer;

class PlayerChunkSender
{
public:
    PlayerChunkSender(ServerPlayer *player);

    void Reset();
    void Tick();
    bool Acknowledge(int count, int renderDistance);
    bool HasSentChunk(const ChunkPos &pos) const;

private:
    void RebuildOffsets(int renderDistance);

    ServerPlayer *m_player;
    std::unordered_set<ChunkPos, ChunkPosHash> m_sentChunks;
    std::vector<ChunkPos> m_streamOffsets;
    ChunkPos m_streamCenter;
    int m_streamRenderDistance;
    size_t m_streamCursor;
    int m_chunksInFlight;
    int m_clientRenderDistance;
};
