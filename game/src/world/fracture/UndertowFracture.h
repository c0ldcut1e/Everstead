#pragma once

#include "FractureTrait.h"

class UndertowFracture : public FractureTrait
{
public:
    UndertowFracture();

    bool OnAttack(ServerPlayer *player, const Hit &hit) override;
};
