#pragma once

#include "CarpetBlock.h"

class PinkPetalsBlock : public CarpetBlock
{
public:
    PinkPetalsBlock(const std::string &texturePath);

    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
};
