#include "SnowLayerBlock.h"

#include <algorithm>

#include "../Level.h"

static const int s_meltLightLevel = 11;

static void MeltSnow(Level *level, const BlockPos &pos)
{
    if (level->GetBlockLightLevel(pos) <= s_meltLightLevel)
    {
        return;
    }

    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}

SnowLayerBlock::SnowLayerBlock(const std::string &texturePath, Block *fullBlock) : LayerBlock(texturePath, fullBlock)
{
    Random random(0x5e0d71f7);
    m_driftNoise = std::make_unique<PerlinNoise>(random, 2);
    SetRandomTicking(true);
}

void SnowLayerBlock::RandomTick(Level *level, const BlockPos &pos)
{
    MeltSnow(level, pos);
}

// This is how deep snow piles up at a column, the world seed shifts the noise so every world gets its own drifts
int SnowLayerBlock::GetDriftLayers(uint32_t seed, int x, int z) const
{
    double offset = (double) (seed % 100000);
    double value  = m_driftNoise->GetValue((x + offset) / 24.0, (z - offset) / 24.0) / 3.0;
    return 1 + (int) std::clamp((value + 0.25) * 6.0, 0.0, 6.0);
}

SnowBlock::SnowBlock(const std::string &texturePath) : Block(true, texturePath)
{
    SetRandomTicking(true);
}

void SnowBlock::RandomTick(Level *level, const BlockPos &pos)
{
    MeltSnow(level, pos);
}
