#include "EmptyBucketDispenseBehavior.h"

#include "../../utils/Direction.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../item/Item.h"

bool EmptyBucketDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    Level *level        = dispenser->GetLevel();
    const BlockPos &pos = dispenser->GetPos();
    BlockPos front(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    Block *block = Block::ById(level->GetBlockId(front));
    if (!block->IsLiquid() || level->GetBlockData(front) != 0)
    {
        return DispenseItemBehavior::Execute(dispenser, facing, item);
    }

    ItemInstance filled(Item::ByName(block->IsLava() ? "lava_bucket" : "water_bucket"), 1);
    level->SetBlock(front, Block::ByName("air"));
    level->RecordBlockChange(front);
    if (item->GetCount() == 1)
    {
        *item = filled;
        return true;
    }

    item->SetCount(item->GetCount() - 1);
    for (int i = 0; i < dispenser->GetSize(); i++)
    {
        if (dispenser->GetItem(i)->IsEmpty())
        {
            dispenser->SetItem(i, filled);
            return true;
        }
    }

    Drop(level, pos, facing, filled);
    return true;
}
