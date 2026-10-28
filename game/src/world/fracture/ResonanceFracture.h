#pragma once

#include "FractureTrait.h"

class ResonanceFracture : public FractureTrait
{
public:
    ResonanceFracture();

    State *CreateState() override;
    bool OnAttack(ServerPlayer *player, const Hit &hit) override;

private:
    struct ResonanceState : State
    {
        int target;
        int hits;
    };
};
