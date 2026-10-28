#pragma once

#include <vector>

#include "Block.h"

class MovingPistonBlock : public Block
{
public:
    MovingPistonBlock();

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetPlacedBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const override;
    void GetSelectionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const override;
    void GetPlacedCollisionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const override;
    const char *GetBlockEntityId() const override;
    void Place(Level *level, const BlockPos &pos, Block *block, uint16_t data, Direction *facing, bool extending, bool source);
};
