#pragma once

#include "FractureTrait.h"

class PhaseFracture : public FractureTrait
{
public:
    PhaseFracture();

    bool PreventHurt(ServerPlayer *player, eDAMAGE_SOURCE source, int pieces) override;
};
