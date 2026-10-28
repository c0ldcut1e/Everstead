#include "TntDispenseBehavior.h"

#include "../../utils/Direction.h"
#include "../block/TntBlock.h"
#include "../blockentity/DispenserBlockEntity.h"

bool TntDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    const BlockPos &pos = dispenser->GetPos();
    TntBlock::Ignite(dispenser->GetLevel(), BlockPos(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz));
    item->SetCount(item->GetCount() - 1);
    return true;
}
