#pragma once

#include "../EntityRenderer.h"

class ChickenRenderer : public EntityRenderer
{
public:
    ChickenRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;
};
