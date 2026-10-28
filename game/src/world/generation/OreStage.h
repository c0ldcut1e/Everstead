#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class OreStage : public WorldGenStage
{
public:
    explicit OreStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
