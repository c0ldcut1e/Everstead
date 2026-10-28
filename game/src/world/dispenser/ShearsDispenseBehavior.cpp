#include "ShearsDispenseBehavior.h"

#include <memory>
#include <vector>

#include "../../entity/animal/Sheep.h"
#include "../../utils/Direction.h"
#include "../Level.h"
#include "../blockentity/DispenserBlockEntity.h"

bool ShearsDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    const BlockPos &pos = dispenser->GetPos();
    glm::dvec3 min(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    AABB box(min, min + glm::dvec3(1.0));
    const std::vector<std::unique_ptr<Entity>> &entities = dispenser->GetLevel()->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Sheep *sheep = dynamic_cast<Sheep *>(entities[i].get());
        if (sheep && sheep->IsShearable() && sheep->GetBoundingBox().Intersects(box))
        {
            sheep->Shear();
            item->ConsumeOne();
            return true;
        }
    }

    return false;
}
