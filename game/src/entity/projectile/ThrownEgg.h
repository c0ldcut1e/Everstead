#pragma once

#include "ThrowableProjectile.h"

class ThrownEgg : public ThrowableProjectile
{
public:
    explicit ThrownEgg(Level *level);

    eENTITY_TYPE GetType() override;
    Item *GetItem() const override;
    void OnRemoved() override;

    static const eENTITY_TYPE s_type = eEntityType_ThrownEgg;

protected:
    void OnHit(Entity *entity) override;
};
