#pragma once

#include <FastNoiseLite.h>

#include "WorldGenStage.h"

class Biome;
class Block;
class TerrainGenerator;

class GlacierStage : public WorldGenStage
{
public:
    explicit GlacierStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    Biome *m_glacier;
    Block *m_air;
    Block *m_packedIce;
    FastNoiseLite m_crevasseNoise;
    FastNoiseLite m_depthNoise;
};
