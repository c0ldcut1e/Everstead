#pragma once

#include "FractureTrait.h"

class RefractionFracture : public FractureTrait
{
public:
    RefractionFracture();

    bool OnArrowHit(ServerPlayer *player, Arrow *arrow, int pieces) override;
};
