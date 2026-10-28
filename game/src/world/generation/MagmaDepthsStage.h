#pragma once

#include "WorldGenStage.h"

class Biome;
class TerrainGenerator;

class MagmaDepthsStage : public WorldGenStage
{
public:
    explicit MagmaDepthsStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
    Biome *m_magmaDepths;
};
