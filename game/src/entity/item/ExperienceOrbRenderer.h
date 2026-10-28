#pragma once

#include "../EntityRenderer.h"

class Texture;

class ExperienceOrbRenderer : public EntityRenderer
{
public:
    ExperienceOrbRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;

private:
    Texture *m_texture;
};
