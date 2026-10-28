#pragma once

#include "../EntityRenderer.h"

class SquidRenderer : public EntityRenderer
{
public:
    SquidRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;
};
