#pragma once

#include "../EntityRenderer.h"

class MinecartRenderer : public EntityRenderer
{
public:
    MinecartRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;
};
