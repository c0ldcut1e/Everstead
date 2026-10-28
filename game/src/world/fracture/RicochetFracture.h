#pragma once

#include "FractureTrait.h"

class RicochetFracture : public FractureTrait
{
public:
    RicochetFracture();

    bool OnArrowLand(Arrow *arrow, Direction *face) override;
};
