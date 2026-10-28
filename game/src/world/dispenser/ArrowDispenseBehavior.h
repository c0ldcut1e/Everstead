#pragma once

#include "ProjectileDispenseBehavior.h"

class ArrowDispenseBehavior : public ProjectileDispenseBehavior
{
protected:
    void Shoot(Level *level, const ItemInstance &item, const glm::dvec3 &position, const glm::dvec3 &velocity) const override;
};
