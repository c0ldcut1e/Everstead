#pragma once

#include "Block.h"

class EnchantedIceBlock : public Block
{
public:
    EnchantedIceBlock(const std::string &texturePath);

    void AnimateTick(Level *level, const BlockPos &pos, Random *random) override;
};
