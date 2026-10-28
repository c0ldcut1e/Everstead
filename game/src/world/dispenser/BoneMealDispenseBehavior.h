#pragma once

#include "DispenseItemBehavior.h"

class BoneMealDispenseBehavior : public DispenseItemBehavior
{
protected:
    bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item) override;
};
