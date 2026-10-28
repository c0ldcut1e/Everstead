#pragma once

#include "Item.h"

class HoeItem : public Item
{
public:
    HoeItem();

    bool CanUseOn(Level *level, const BlockPos &pos, Direction *face) const override;
    void UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const override;
};
