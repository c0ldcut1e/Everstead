#pragma once

#include "ThrowableItem.h"

class SnowballItem : public ThrowableItem
{
public:
    ThrowableProjectile *CreateProjectile(Level *level) const override;
};
