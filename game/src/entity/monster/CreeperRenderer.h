#pragma once

#include "../animal/QuadrupedRenderer.h"

class CreeperRenderer : public QuadrupedRenderer
{
public:
    CreeperRenderer();

protected:
    void ApplyTransform(const Mob *mob, float partialTicks, glm::dmat4 *matrix) const override;
    uint32_t GetOverlayColor(const Mob *mob, float partialTicks) const override;
};
