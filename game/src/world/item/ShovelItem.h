#pragma once

#include "DiggerItem.h"

class ShovelItem : public DiggerItem
{
public:
    ShovelItem();

    bool CanUseOn(Level *level, const BlockPos &pos, Direction *face) const override;
    void UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const override;
};
