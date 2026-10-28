#pragma once

#include "FractureTrait.h"

class SplinterFracture : public FractureTrait
{
public:
    SplinterFracture();

    State *CreateState() override;
    void Tick(ServerPlayer *player, State *state) override;
    bool OnAttack(ServerPlayer *player, const Hit &hit) override;

private:
    struct SplinterState : State
    {
        int target;
        int ticks;
        int damage;
    };
};
