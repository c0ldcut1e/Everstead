#pragma once

#include "Entity.h"

class EntityRenderer
{
public:
    EntityRenderer();
    virtual ~EntityRenderer() = default;

    virtual void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const;

    void SetRenderBoundingBox(bool value);
    float GetShadowRadius() const;

protected:
    void RenderBoundingBox(const Entity *entity, float partialTicks) const;
    void RenderFire(const Entity *entity, float partialTicks) const;

    bool m_renderBoundingBox;
    float m_shadowRadius;
};
