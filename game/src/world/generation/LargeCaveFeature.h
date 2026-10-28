#pragma once

#include "LargeFeature.h"

class LargeCaveFeature : public LargeFeature
{
public:
    LargeCaveFeature(uint32_t seed, int genDepth);

    void Apply(Chunk &chunk, const ChunkPos &chunkPos) override;

private:
    bool AddFeature(Chunk &chunk, const ChunkPos &chunkPos, Random &random, int featureX, int featureZ);
    void AddRoom(Chunk &chunk, const ChunkPos &chunkPos, Random &featureRandom, uint64_t seed, double xRoom, double yRoom, double zRoom);
    void AddTunnel(Chunk &chunk, const ChunkPos &chunkPos, uint64_t seed, double xCave, double yCave, double zCave, float thickness, float yRot,
                   float xRot, int step, int dist, double yScale, bool flooded);

    static const int s_radius = 8;

    int m_genDepth;
    Block *m_air;
    Block *m_lava;
    Block *m_stone;
    Block *m_dirt;
    Block *m_grassBlock;
    Block *m_sand;
    Block *m_waterStill;
    Block *m_waterFlow;
};
