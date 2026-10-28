#pragma once

#include "FractureTrait.h"

class RecallFracture : public FractureTrait
{
public:
    RecallFracture();

    bool OnArrowLand(Arrow *arrow, Direction *face) override;
};
