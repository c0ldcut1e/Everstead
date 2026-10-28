#pragma once

#include "QuadrupedRenderer.h"

class SheepRenderer : public QuadrupedRenderer
{
public:
    SheepRenderer();

protected:
    void PoseModel(const Mob *mob, Model *model, float partialTicks) const override;
    void RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const override;
};
