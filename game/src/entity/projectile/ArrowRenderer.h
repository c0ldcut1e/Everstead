#pragma once

#include "../EntityRenderer.h"

class Texture;

class ArrowRenderer : public EntityRenderer
{
public:
    ArrowRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;

private:
    Texture *m_texture;
};
