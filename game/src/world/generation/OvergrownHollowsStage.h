#pragma once

#include <FastNoiseLite.h>

#include "WorldGenStage.h"

class Biome;
class TerrainGenerator;

class OvergrownHollowsStage : public WorldGenStage
{
public:
    explicit OvergrownHollowsStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
    Biome *m_overgrownHollows;
    FastNoiseLite m_rubbleNoise;
    FastNoiseLite m_mossNoise;
    FastNoiseLite m_poolNoise;
};
