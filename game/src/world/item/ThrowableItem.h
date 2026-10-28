#pragma once

#include "../../entity/projectile/ThrowableProjectile.h"
#include "Item.h"

class ThrowableItem : public Item
{
public:
    virtual ThrowableProjectile *CreateProjectile(Level *level) const = 0;
};
