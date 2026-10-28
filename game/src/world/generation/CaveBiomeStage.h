#pragma once

#include <FastNoiseLite.h>

#include "WorldGenStage.h"

class Biome;
class TerrainGenerator;

class CaveBiomeStage : public WorldGenStage
{
public:
    explicit CaveBiomeStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
    Biome *m_cave;
    Biome *m_dripstoneCaves;
    Biome *m_iceCaves;
    Biome *m_desertCaves;
    Biome *m_overgrownHollows;
    Biome *m_glacier;
    Biome *m_magmaDepths;
    FastNoiseLite m_dripstoneNoise;
    FastNoiseLite m_iceCaveNoise;
    FastNoiseLite m_magmaNoise;
};
