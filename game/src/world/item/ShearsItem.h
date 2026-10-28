#pragma once

#include "Item.h"

class ShearsItem : public Item
{
public:
    ShearsItem();

    float GetDestroySpeed(Block *block) const override;
    int GetMineDamage(Block *block) const override;
    bool CanUseOn(Level *level, const BlockPos &pos, Direction *face) const override;
    void UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const override;
};
