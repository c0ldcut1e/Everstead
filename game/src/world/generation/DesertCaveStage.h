#pragma once

#include "WorldGenStage.h"

class Biome;
class TerrainGenerator;

class DesertCaveStage : public WorldGenStage
{
public:
    explicit DesertCaveStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
    Biome *m_desertCaves;
};
