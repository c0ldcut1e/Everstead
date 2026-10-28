#pragma once

#include "Block.h"

class SunbloomBlock : public Block
{
public:
    SunbloomBlock(const std::string &texturePath);

    uint16_t GetPlacementData(Direction *placedAgainst) const override;
    Direction *GetAttachment(uint16_t data) const override;
    uint16_t GetItemData() const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    AABB GetPlacedAABB(const BlockPos &pos, uint16_t data) const override;

private:
    int m_faceProperty;
};
