#pragma once

#include "Block.h"

class CarpetBlock : public Block
{
public:
    CarpetBlock(const std::string &texturePath);

    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
};
