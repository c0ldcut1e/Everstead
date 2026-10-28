#pragma once

#include "FractureTrait.h"

class SeamFracture : public FractureTrait
{
public:
    SeamFracture();

    int OnHarvest(ServerPlayer *player, const Harvest &harvest) override;
};
