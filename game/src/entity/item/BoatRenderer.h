#pragma once

#include "../EntityRenderer.h"

#include <glm/mat4x4.hpp>

class Boat;

class BoatRenderer : public EntityRenderer
{
public:
    BoatRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;
    void RenderWaterPatch(const Boat *boat, float partialTicks) const;

private:
    glm::dmat4 PushTransform(const Boat *boat, float partialTicks) const;
};
