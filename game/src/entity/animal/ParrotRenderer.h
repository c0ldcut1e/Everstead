#pragma once

#include "QuadrupedRenderer.h"

class ParrotRenderer : public QuadrupedRenderer
{
public:
    ParrotRenderer();

protected:
    void PoseModel(const Mob *mob, Model *model, float partialTicks) const override;
    void RenderModel(const Mob *mob, Model *model, uint32_t color, const glm::dmat4 &matrix) const override;
};
