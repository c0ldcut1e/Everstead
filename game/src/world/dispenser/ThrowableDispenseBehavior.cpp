#include "ThrowableDispenseBehavior.h"

#include <memory>

#include "../Level.h"
#include "../item/ThrowableItem.h"

void ThrowableDispenseBehavior::Shoot(Level *level, const ItemInstance &item, const glm::dvec3 &position, const glm::dvec3 &velocity) const
{
    std::unique_ptr<Entity> projectile(((ThrowableItem *) item.GetItem())->CreateProjectile(level));
    projectile->SetPosition(position);
    projectile->SetVelocity(velocity);
    level->AddEntity(std::move(projectile));
}
