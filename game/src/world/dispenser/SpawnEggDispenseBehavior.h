#pragma once

#include "DispenseItemBehavior.h"

class SpawnEggDispenseBehavior : public DispenseItemBehavior
{
protected:
    bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item) override;
};
