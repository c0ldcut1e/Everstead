#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class StoneVariantStage : public WorldGenStage
{
public:
    explicit StoneVariantStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
