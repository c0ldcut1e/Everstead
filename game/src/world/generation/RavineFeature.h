#pragma once

#include "LargeFeature.h"

class RavineFeature : public LargeFeature
{
public:
    RavineFeature(uint32_t seed, int genDepth);

    void Apply(Chunk &chunk, const ChunkPos &chunkPos) override;

private:
    void AddFeature(Chunk &chunk, const ChunkPos &chunkPos, Random &random, int featureX, int featureZ);
    void AddTunnel(Chunk &chunk, const ChunkPos &chunkPos, uint64_t seed, double xCave, double yCave, double zCave, float thickness, float yRot,
                   float xRot, double yScale);

    static const int s_radius = 8;

    int m_genDepth;
    Block *m_air;
    Block *m_lava;
    Block *m_stone;
    Block *m_dirt;
    Block *m_grassBlock;
    Block *m_sand;
    Block *m_waterStill;
};
