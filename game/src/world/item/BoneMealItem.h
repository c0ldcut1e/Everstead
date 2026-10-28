#pragma once

#include "Item.h"

class BoneMealItem : public Item
{
public:
    BoneMealItem();

    bool CanUseOn(Level *level, const BlockPos &pos, Direction *face) const override;
    void UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const override;
    void SpawnUseEffects(Level *level, const BlockPos &pos) const override;
};
