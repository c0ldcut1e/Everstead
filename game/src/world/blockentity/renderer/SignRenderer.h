#pragma once

#include <cstdint>

#include "BlockEntityRenderer.h"

#include <glm/mat4x4.hpp>

class BufferBuilder;
class SignBlock;

class SignRenderer : public BlockEntityRenderer
{
public:
    void Render(const BlockEntity *blockEntity, float partialTicks) const override;
    void RenderItem(uint32_t color) const override;

private:
    void RenderModel(double x, double y, double z, SignBlock *block, float yRot, uint32_t color) const;
    void RenderBox(BufferBuilder *builder, const glm::dmat4 &matrix, uint32_t color, float boxX, float boxY, float boxZ, int w, int h, int d,
                   int texU, int texV) const;
};
