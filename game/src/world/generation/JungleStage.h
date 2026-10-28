#pragma once

#include "WorldGenStage.h"

class TerrainGenerator;

class JungleStage : public WorldGenStage
{
public:
    explicit JungleStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
};
