#pragma once

#include "../EntityRenderer.h"

class ItemEntityRenderer : public EntityRenderer
{
public:
    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;

    static const Entity *FindCollector(uint8_t playerIndex);
};
