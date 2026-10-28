#pragma once

#include "QuadrupedRenderer.h"

class OcelotRenderer : public QuadrupedRenderer
{
public:
    OcelotRenderer();

protected:
    void PoseModel(const Mob *mob, Model *model, float partialTicks) const override;
};
