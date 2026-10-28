#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class LilyPadStage : public WorldGenStage
{
public:
    explicit LilyPadStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
