#pragma once

#include "Block.h"

class BambooSaplingBlock : public Block
{
public:
    BambooSaplingBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    void RandomTick(Level *level, const BlockPos &pos) override;
};
