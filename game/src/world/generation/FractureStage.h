#pragma once

#include "WorldGenStage.h"

class Block;
class TerrainGenerator;

class FractureStage : public WorldGenStage
{
public:
    explicit FractureStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
    Block *m_air;
    Block *m_bedrock;
    Block *m_frame;
    Block *m_socket;
};
