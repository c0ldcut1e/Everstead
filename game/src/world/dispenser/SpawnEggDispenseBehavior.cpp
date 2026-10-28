#include "SpawnEggDispenseBehavior.h"

#include "../../utils/Direction.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../item/SpawnEggItem.h"

bool SpawnEggDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    const BlockPos &pos = dispenser->GetPos();
    ((SpawnEggItem *) item->GetItem())
            ->Spawn(dispenser->GetLevel(), glm::dvec3(pos.x + facing->dx + 0.5, pos.y + facing->dy + 0.2, pos.z + facing->dz + 0.5));
    item->SetCount(item->GetCount() - 1);
    return true;
}
