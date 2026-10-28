#ifndef EVERSTEAD_DEDICATED_SERVER

#include "LeashKnotRenderer.h"

#include "../../rendering/GlStateManager.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/ModelRegistry.h"

#include <glm/gtc/matrix_transform.hpp>

void LeashKnotRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    glm::dvec3 pos    = entity->GetRenderPosition(partialTicks);
    const float scale = 1.0f / 16.0f;
    glm::dmat4 matrix = glm::scale(glm::translate(glm::dmat4(1.0), pos), glm::dvec3(scale, scale, scale));
    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x, pos.y, pos.z);
    GlStateManager::Scalef(scale, scale, scale);
    ModelRegistry::GetInstance()
            ->GetModel("leash_knot")
            ->Render(*ModelRegistry::GetInstance()->GetTextures(), Lighting::IsEnabled() ? 0xFFFFFFFF : lightColor, matrix);
    GlStateManager::PopMatrix();

    if (m_renderBoundingBox)
    {
        RenderBoundingBox(entity, partialTicks);
    }
}

#endif
