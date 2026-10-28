#pragma once

#include "DispenseItemBehavior.h"

class ArmorDispenseBehavior : public DispenseItemBehavior
{
protected:
    bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item) override;
};
