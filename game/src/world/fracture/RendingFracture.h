#pragma once

#include "FractureTrait.h"

class RendingFracture : public FractureTrait
{
public:
    RendingFracture();

    int ModifyAttackDamage(ServerPlayer *player, int damage) override;
    int GetAttackWear() override;
};
