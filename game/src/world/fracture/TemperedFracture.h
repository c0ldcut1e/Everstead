#pragma once

#include "FractureTrait.h"

class TemperedFracture : public FractureTrait
{
public:
    TemperedFracture();

    int ModifyWear(int amount, Random *random) override;
};
