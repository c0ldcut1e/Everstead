#pragma once

#include "DispenseItemBehavior.h"

class EmptyBucketDispenseBehavior : public DispenseItemBehavior
{
protected:
    bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item) override;
};
