#pragma once

#include "FractureTrait.h"

class OverflowFracture : public FractureTrait
{
public:
    OverflowFracture();

    int OnHarvest(ServerPlayer *player, const Harvest &harvest) override;
};
