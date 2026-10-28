#pragma once

#include <memory>

#include "../generation/PerlinNoise.h"
#include "LayerBlock.h"

class SnowLayerBlock : public LayerBlock
{
public:
    SnowLayerBlock(const std::string &texturePath, Block *fullBlock);

    void RandomTick(Level *level, const BlockPos &pos) override;

    int GetDriftLayers(uint32_t seed, int x, int z) const;

private:
    std::unique_ptr<PerlinNoise> m_driftNoise;
};

class SnowBlock : public Block
{
public:
    SnowBlock(const std::string &texturePath);

    void RandomTick(Level *level, const BlockPos &pos) override;
};
