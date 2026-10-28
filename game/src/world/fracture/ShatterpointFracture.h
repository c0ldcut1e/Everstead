#pragma once

#include "FractureTrait.h"

class ShatterpointFracture : public FractureTrait
{
public:
    ShatterpointFracture();

    State *CreateState() override;
    int ModifyAttackDamage(ServerPlayer *player, int damage) override;
    bool OnAttack(ServerPlayer *player, const Hit &hit) override;

private:
    struct ShatterpointState : State
    {
        bool shattered;
    };
};
