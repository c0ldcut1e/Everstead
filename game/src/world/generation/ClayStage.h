#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class ClayStage : public WorldGenStage
{
public:
    explicit ClayStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
