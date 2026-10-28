#pragma once

#include "ThrowableProjectile.h"

class Snowball : public ThrowableProjectile
{
public:
    explicit Snowball(Level *level);

    eENTITY_TYPE GetType() override;
    Item *GetItem() const override;
    void OnRemoved() override;

    static const eENTITY_TYPE s_type = eEntityType_Snowball;

protected:
    void OnHit(Entity *entity) override;
};
