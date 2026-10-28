#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class ForestRockStage : public WorldGenStage
{
public:
    explicit ForestRockStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
