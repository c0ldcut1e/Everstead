#pragma once

#include "../../utils/Random.h"
#include "WorldGenStage.h"

class TerrainGenerator;

class IceSpikeStage : public WorldGenStage
{
public:
    explicit IceSpikeStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    void PlaceIcePatch(Chunk &chunk, const ChunkPos &pos, Random &random, int levelX, int surface, int levelZ);
    void PlaceIceSpike(Chunk &chunk, const ChunkPos &pos, Random &random, int levelX, int surface, int levelZ);

    TerrainGenerator *m_generator;
};
