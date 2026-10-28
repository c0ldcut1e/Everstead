#pragma once

#include "DispenseItemBehavior.h"

class TntDispenseBehavior : public DispenseItemBehavior
{
protected:
    bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item) override;
};
