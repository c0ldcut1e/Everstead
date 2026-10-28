#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MobSpawnerRenderer.h"

#include "../../../entity/EntityRenderer.h"
#include "../../../entity/EntityRendererRegistry.h"
#include "../../../rendering/GlStateManager.h"
#include "../../lighting/Lighting.h"
#include "../MobSpawnerBlockEntity.h"

void MobSpawnerRenderer::Render(const BlockEntity *blockEntity, float partialTicks) const
{
    MobSpawnerBlockEntity *spawner = (MobSpawnerBlockEntity *) blockEntity;
    const BlockPos &pos            = spawner->GetPos();
    Entity *entity                 = spawner->GetDisplayEntity();
    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x + 0.5, pos.y, pos.z + 0.5);
    GlStateManager::Translatef(0.0f, 0.4f, 0.0f);
    GlStateManager::Rotatef(spawner->GetSpin(partialTicks) * 10.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(-30.0f, 1.0f, 0.0f, 0.0f);
    GlStateManager::Translatef(0.0f, -0.4f, 0.0f);
    GlStateManager::Scalef(0.4375f, 0.4375f, 0.4375f);
    EntityRendererRegistry::GetInstance()->GetValue(entity->GetType())->Render(entity, partialTicks, Lighting::GetEntityLight());
    GlStateManager::PopMatrix();
}

void MobSpawnerRenderer::RenderItem(uint32_t color) const
{
    (void) color;
}

#endif
