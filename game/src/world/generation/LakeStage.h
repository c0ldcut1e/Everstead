#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class LakeStage : public WorldGenStage
{
public:
    explicit LakeStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
