#pragma once

#include "Block.h"

class IceBlock : public Block
{
public:
    static constexpr int s_meltLightLevel = 11;

    IceBlock(const std::string &texturePath);

    void RandomTick(Level *level, const BlockPos &pos) override;
};
