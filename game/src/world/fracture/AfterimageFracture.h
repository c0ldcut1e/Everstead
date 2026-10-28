#pragma once

#include "FractureTrait.h"

class AfterimageFracture : public FractureTrait
{
public:
    AfterimageFracture();

    void OnHurt(ServerPlayer *player, int pieces) override;
};
