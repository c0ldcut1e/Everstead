#pragma once

#include "DispenseItemBehavior.h"

class ProjectileDispenseBehavior : public DispenseItemBehavior
{
protected:
    bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item) override;
    void PlaySound(Level *level, const BlockPos &pos, bool success) const override;
    virtual void Shoot(Level *level, const ItemInstance &item, const glm::dvec3 &position, const glm::dvec3 &velocity) const = 0;
};
