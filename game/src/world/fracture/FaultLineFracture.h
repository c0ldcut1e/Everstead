#pragma once

#include "FractureTrait.h"

class FaultLineFracture : public FractureTrait
{
public:
    FaultLineFracture();

    int OnHarvest(ServerPlayer *player, const Harvest &harvest) override;
};
