#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SkeletonRenderer.h"

#include "../../rendering/GlStateManager.h"
#include "../../world/ItemInHandRenderer.h"
#include "../../world/item/Item.h"
#include "../../world/models/Model.h"
#include "../../world/models/ModelRegistry.h"
#include "../Mob.h"
#include "../player/PlayerRenderer.h"

SkeletonRenderer::SkeletonRenderer() : ZombieRenderer("skeleton")
{}

void SkeletonRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.003f);
    ZombieRenderer::Render(entity, partialTicks, lightColor);
    GlStateManager::SetAlphaCutoff(alphaCutoff);
}

void SkeletonRenderer::RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const
{
    (void) mob;
    (void) color;
    (void) matrix;
    (void) partialTicks;
    ModelPart *rightArm           = ModelRegistry::GetInstance()->GetModel("skeleton")->FindPart("rightArm");
    const glm::dvec3 &armPosition = rightArm->GetPosition();
    const glm::dvec3 &armPivot    = rightArm->GetPivot();
    const glm::dvec3 &armRotation = rightArm->GetRotation();
    Item *bow                     = Item::ByName("bow");

    GlStateManager::PushMatrix();
    GlStateManager::Translatef((float) armPosition.x, (float) armPosition.y, (float) armPosition.z);
    GlStateManager::Translatef((float) armPivot.x, (float) armPivot.y, (float) armPivot.z);
    GlStateManager::Rotatef((float) armRotation.z, 0.0f, 0.0f, 1.0f);
    GlStateManager::Rotatef((float) armRotation.y, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef((float) armRotation.x, 1.0f, 0.0f, 0.0f);
    GlStateManager::Translatef((float) -armPivot.x, (float) -armPivot.y, (float) -armPivot.z);
    GlStateManager::Translatef(PlayerRenderer::s_heldItemArmOffsetX, PlayerRenderer::s_heldItemArmOffsetY, PlayerRenderer::s_heldItemArmOffsetZ);
    GlStateManager::Translatef(0.0f, -2.0f, -5.0f);
    GlStateManager::Rotatef(20.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Scalef(PlayerRenderer::s_heldItemScale, PlayerRenderer::s_heldItemScale, -PlayerRenderer::s_heldItemScale);
    GlStateManager::Rotatef(-100.0f, 1.0f, 0.0f, 0.0f);
    GlStateManager::Rotatef(45.0f, 0.0f, 1.0f, 0.0f);
    ItemInHandRenderer::RenderItem(bow, bow->GetIcon(), lightColor);
    GlStateManager::PopMatrix();
}

#endif
