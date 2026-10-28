#include "ArrowDispenseBehavior.h"

#include <memory>
#include <utility>

#include "../../entity/projectile/Arrow.h"
#include "../Level.h"

void ArrowDispenseBehavior::Shoot(Level *level, const ItemInstance &item, const glm::dvec3 &position, const glm::dvec3 &velocity) const
{
    (void) item;

    std::unique_ptr<Arrow> arrow(new Arrow(level));
    arrow->Launch(position, velocity);
    level->AddEntity(std::move(arrow));
}
