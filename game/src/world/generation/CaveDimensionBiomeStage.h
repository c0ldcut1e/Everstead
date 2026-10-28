#pragma once

#include "WorldGenStage.h"

#include <FastNoiseLite.h>

class Biome;

class CaveDimensionBiomeStage : public WorldGenStage
{
public:
    explicit CaveDimensionBiomeStage(uint32_t seed);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    Biome *m_cave;
    Biome *m_dripstoneCaves;
    Biome *m_iceCaves;
    Biome *m_desertCaves;
    Biome *m_overgrownHollows;
    Biome *m_magmaDepths;
    FastNoiseLite m_temperatureNoise;
    FastNoiseLite m_vegetationNoise;
};
