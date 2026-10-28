#include "ProjectileDispenseBehavior.h"

#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../Level.h"
#include "../blockentity/DispenserBlockEntity.h"

#include <glm/geometric.hpp>

bool ProjectileDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    Random &random       = dispenser->GetLevel()->GetRandom();
    glm::dvec3 direction = glm::normalize(glm::dvec3(facing->dx, facing->dy + 0.1, facing->dz));
    direction += glm::dvec3(random.NextGaussian(), random.NextGaussian(), random.NextGaussian()) * (0.0075 * 6.0);
    Shoot(dispenser->GetLevel(), *item, GetDispensePosition(dispenser->GetPos(), facing), direction * 1.1);
    item->SetCount(item->GetCount() - 1);
    return true;
}

void ProjectileDispenseBehavior::PlaySound(Level *level, const BlockPos &pos, bool success) const
{
    (void) success;

    level->PlaySound("random.bow", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f, 1.2f);
}
