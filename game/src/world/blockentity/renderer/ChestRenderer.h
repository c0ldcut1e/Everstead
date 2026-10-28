#pragma once

#include <cstdint>

#include "BlockEntityRenderer.h"

#include <glm/mat4x4.hpp>

class BufferBuilder;
class Texture;

class ChestRenderer : public BlockEntityRenderer
{
public:
    ChestRenderer();

    void Render(const BlockEntity *blockEntity, float partialTicks) const override;
    void RenderItem(uint32_t color) const override;

private:
    void RenderModel(double x, double y, double z, int facing, bool large, float openness, uint32_t color) const;
    void RenderBox(BufferBuilder *builder, const glm::dmat4 &matrix, uint32_t color, float xRot, float pivotX, float pivotY, float pivotZ, float boxX,
                   float boxY, float boxZ, int w, int h, int d, int texU, int texV, float texWidth) const;

    Texture *m_texture;
    Texture *m_largeTexture;
};
