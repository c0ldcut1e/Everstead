#pragma once

#include <FastNoiseLite.h>

#include "../../utils/Random.h"
#include "WorldGenStage.h"

class Biome;
class TerrainGenerator;

class IceCaveStage : public WorldGenStage
{
public:
    explicit IceCaveStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    void PlaceIceCrystal(Chunk &chunk, Random &random, int x, int y, int z) const;
    void PlaceBrittleIcePit(Chunk &chunk, Random &random, int x, int y, int z) const;

    TerrainGenerator *m_generator;
    Biome *m_iceCaves;
    Biome *m_glacier;
    FastNoiseLite m_iceZoneNoise;
};
