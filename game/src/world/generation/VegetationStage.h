#pragma once

#include <FastNoiseLite.h>

#include "../../utils/Random.h"
#include "WorldGenStage.h"

class Biome;
class Block;
class DoublePlantBlock;
class TerrainGenerator;

class VegetationStage : public WorldGenStage
{
public:
    explicit VegetationStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    void PlaceDoublePlant(Chunk &chunk, Random &random, DoublePlantBlock *plant, Biome *biome) const;
    void PlaceFlowers(Chunk &chunk, Random &random, Biome *biome) const;
    void PlaceNoiseFlowers(Chunk &chunk, Random &random, Biome *biome) const;
    void PlacePatch(Chunk &chunk, Random &random, Block *block, int spread, int attempts, int minY, int maxY) const;

    TerrainGenerator *m_generator;
    FastNoiseLite m_flowerNoise;
    FastNoiseLite m_pumpkinNoise;
};
