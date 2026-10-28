#pragma once

#include "Block.h"

class LilyPadBlock : public Block
{
public:
    LilyPadBlock();

    bool HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
};
