#pragma once

#include <memory>

#include "PerlinNoise.h"
#include "WorldGenStage.h"

class TerrainGenerator;

class SandLayerStage : public WorldGenStage
{
public:
    explicit SandLayerStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    TerrainGenerator *m_generator;
    std::unique_ptr<PerlinNoise> m_duneNoise;
};
