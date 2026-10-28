#pragma once

#include "WorldGenStage.h"

class Block;
class TerrainGenerator;

class SpringStage : public WorldGenStage
{
public:
    SpringStage(TerrainGenerator *generator, Block *liquid, bool lava);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
    Block *m_liquid;
    bool m_lava;
};
