#pragma once

#include <cstdint>

class BlockEntity;

class BlockEntityRenderer
{
public:
    virtual ~BlockEntityRenderer() = default;

    virtual void Render(const BlockEntity *blockEntity, float partialTicks) const = 0;
    virtual void RenderItem(uint32_t color) const                                 = 0;
};
