#pragma once

#include "ZombieRenderer.h"

class SkeletonRenderer : public ZombieRenderer
{
public:
    SkeletonRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;

protected:
    void RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const override;
};
