#pragma once

#include "WorldGenStage.h"

class Biome;
class Direction;
class PointedDripstoneBlock;
class TerrainGenerator;

class DripstoneStage : public WorldGenStage
{
public:
    explicit DripstoneStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

    static void PlaceColumn(Chunk &chunk, PointedDripstoneBlock *block, int x, int y, int z, Direction *tipDirection, int length);

private:
    TerrainGenerator *m_generator;
    Biome *m_dripstoneCaves;
};
