#pragma once

#include "../animal/QuadrupedRenderer.h"

class SpiderRenderer : public QuadrupedRenderer
{
public:
    SpiderRenderer();

protected:
    void PoseModel(const Mob *mob, Model *model, float partialTicks) const override;
    void RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const override;
};
