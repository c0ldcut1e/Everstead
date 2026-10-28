#ifndef EVERSTEAD_DEDICATED_SERVER

#include "PrimedTntRenderer.h"

#include <algorithm>

#include "../../rendering/GlStateManager.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRenderer.h"
#include "PrimedTnt.h"

void PrimedTntRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const PrimedTnt *tnt = (const PrimedTnt *) entity;
    glm::dvec3 position  = tnt->GetRenderPosition(partialTicks);
    float life           = tnt->GetFuse() - partialTicks + 1.0f;
    GlStateManager::PushMatrix();
    GlStateManager::Translated(position.x, position.y + 0.5, position.z);
    if (life < 10.0f)
    {
        float g = std::clamp(1.0f - life / 10.0f, 0.0f, 1.0f);
        g       = g * g * g * g;
        float s = 1.0f + g * 0.3f;
        GlStateManager::Scalef(s, s, s);
    }

    if (tnt->GetFuse() / 5 % 2 == 0)
    {
        float brightness = std::clamp((1.0f - life / 100.0f) * 0.8f, 0.0f, 1.0f);
        GlStateManager::SetOverlayColor(((uint32_t) (brightness * 255.0f) << 24) | 0xFFFFFF);
    }

    BlockRenderer::RenderBlockAsEntity(Block::ByName("tnt"), 0, lightColor);
    GlStateManager::SetOverlayColor(0);
    GlStateManager::PopMatrix();
    EntityRenderer::Render(entity, partialTicks, lightColor);
}

#endif
