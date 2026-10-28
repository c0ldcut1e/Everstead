#include "BucketDispenseBehavior.h"

#include "../../utils/Direction.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../item/BucketItem.h"

bool BucketDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    BucketItem *bucket  = (BucketItem *) item->GetItem();
    const BlockPos &pos = dispenser->GetPos();
    if (!bucket->CanUse(dispenser->GetLevel(), pos, facing))
    {
        return DispenseItemBehavior::Execute(dispenser, facing, item);
    }

    bucket->PlaceLiquid(dispenser->GetLevel(), BlockPos(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz));
    *item = ItemInstance(Item::ByName("bucket"), 1);
    return true;
}
