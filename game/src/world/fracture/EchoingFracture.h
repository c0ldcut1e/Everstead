#pragma once

#include "FractureTrait.h"

class EchoingFracture : public FractureTrait
{
public:
    EchoingFracture();

    bool PreventBreak(ServerPlayer *player, Level *level, const BlockPos &pos) override;
    int OnHarvest(ServerPlayer *player, const Harvest &harvest) override;
};
