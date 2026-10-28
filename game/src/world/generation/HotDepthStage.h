#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class HotDepthStage : public WorldGenStage
{
public:
    explicit HotDepthStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
