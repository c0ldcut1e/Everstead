#pragma once

#include "ThrowableItem.h"

class EggItem : public ThrowableItem
{
public:
    ThrowableProjectile *CreateProjectile(Level *level) const override;
};
