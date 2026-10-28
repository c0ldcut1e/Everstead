#ifndef EVERSTEAD_DEDICATED_SERVER

#include "FallingBlockRenderer.h"

#include "../../rendering/GlStateManager.h"
#include "../../world/block/BlockRenderer.h"
#include "FallingBlock.h"

void FallingBlockRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const FallingBlock *fallingBlock = (const FallingBlock *) entity;
    if (!fallingBlock->GetBlock())
    {
        return;
    }

    glm::dvec3 position = fallingBlock->GetRenderPosition(partialTicks);
    GlStateManager::PushMatrix();
    GlStateManager::Translated(position.x, position.y + 0.5, position.z);
    BlockRenderer::RenderBlockAsEntity(fallingBlock->GetBlock(), fallingBlock->GetBlockData(), lightColor);
    GlStateManager::PopMatrix();
    EntityRenderer::Render(entity, partialTicks, lightColor);
}

#endif
