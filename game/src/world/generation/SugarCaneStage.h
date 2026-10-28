#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class SugarCaneStage : public WorldGenStage
{
public:
    explicit SugarCaneStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
