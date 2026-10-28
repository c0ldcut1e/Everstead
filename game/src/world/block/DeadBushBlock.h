#pragma once

#include "Block.h"

class DeadBushBlock : public Block
{
public:
    DeadBushBlock(const std::string &texturePath);

    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
};
