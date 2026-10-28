#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class SnowStage : public WorldGenStage
{
public:
    explicit SnowStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
