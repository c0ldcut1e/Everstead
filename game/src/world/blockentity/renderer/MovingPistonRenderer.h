#pragma once

#include <cstdint>

#include "BlockEntityRenderer.h"

class MovingPistonRenderer : public BlockEntityRenderer
{
public:
    void Render(const BlockEntity *blockEntity, float partialTicks) const override;
    void RenderItem(uint32_t color) const override;
};
