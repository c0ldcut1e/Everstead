#pragma once

#include <FastNoiseLite.h>

#include "WorldGenStage.h"

class Biome;
class Block;
class TerrainGenerator;

class BadlandsStage : public WorldGenStage
{
public:
    explicit BadlandsStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    static const int s_bandCount = 64;

    Biome *m_badlands;
    Biome *m_woodedBadlands;
    Block *m_bands[s_bandCount];
    FastNoiseLite m_bandNoise;
};
