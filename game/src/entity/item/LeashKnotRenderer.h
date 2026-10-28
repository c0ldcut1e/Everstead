#pragma once

#include "../EntityRenderer.h"

class LeashKnotRenderer : public EntityRenderer
{
public:
    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;
};
