#include "BoneMealDispenseBehavior.h"

#include "../../utils/Direction.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../item/BoneMealItem.h"

bool BoneMealDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    BoneMealItem *boneMeal = (BoneMealItem *) item->GetItem();
    const BlockPos &pos    = dispenser->GetPos();
    BlockPos front(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    if (!boneMeal->CanUseOn(dispenser->GetLevel(), front, facing))
    {
        return false;
    }

    boneMeal->SpawnUseEffects(dispenser->GetLevel(), front);
    boneMeal->UseOn(dispenser->GetLevel(), front, facing, glm::dvec3(facing->dx, facing->dy, facing->dz));
    item->SetCount(item->GetCount() - 1);
    return true;
}
