#pragma once

#include "Block.h"

class GravityBlock : public Block
{
public:
    GravityBlock(const std::string &texturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;

    static bool CanFallThrough(Block *block);
    static bool CanFallInto(Block *block);
};
