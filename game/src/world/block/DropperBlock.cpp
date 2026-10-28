#include "DropperBlock.h"

#include "../Level.h"
#include "../blockentity/DropperBlockEntity.h"
#include "../dispenser/DispenseItemBehavior.h"

DropperBlock::DropperBlock() : DispenserBlock("assets/texture/block/dropper_front.png", "assets/texture/block/dropper_front_vertical.png")
{}

const char *DropperBlock::GetBlockEntityId() const
{
    return "dropper";
}

void DropperBlock::DispenseFrom(Level *level, const BlockPos &pos, DispenserBlockEntity *dispenser, int slot)
{
    Direction *facing = GetFacing(level->GetBlockData(pos));
    if (((DropperBlockEntity *) dispenser)->InsertInFront(facing, slot))
    {
        return;
    }

    dispenser->SetItem(slot, DispenseItemBehavior::GetDefault()->Dispense(dispenser, facing, *dispenser->GetItem(slot)));
}
