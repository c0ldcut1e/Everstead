#pragma once

#include "DispenseItemBehavior.h"

class BoatDispenseBehavior : public DispenseItemBehavior
{
protected:
    bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item) override;
};
