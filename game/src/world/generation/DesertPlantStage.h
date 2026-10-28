#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class DesertPlantStage : public WorldGenStage
{
public:
    explicit DesertPlantStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
