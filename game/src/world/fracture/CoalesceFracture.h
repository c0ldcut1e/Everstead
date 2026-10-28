#pragma once

#include "FractureTrait.h"

class CoalesceFracture : public FractureTrait
{
public:
    CoalesceFracture();

    int OnPickupExperience(ServerPlayer *player, ItemInstance *item, int amount) override;
};
