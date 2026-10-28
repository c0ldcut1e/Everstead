#pragma once

#include "FractureTrait.h"

class TearFracture : public FractureTrait
{
public:
    TearFracture();

    State *CreateState() override;
    void Tick(ServerPlayer *player, State *state) override;
    bool OnAttack(ServerPlayer *player, const Hit &hit) override;

private:
    struct TearState : State
    {
        glm::dvec3 position;
        int ticks;
    };
};
