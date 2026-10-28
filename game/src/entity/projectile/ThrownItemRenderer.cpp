#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ThrownItemRenderer.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Texture.h"
#include "../../rendering/Textures.h"
#include "../../scene/Camera.h"
#include "../../world/item/Item.h"
#include "ThrowableProjectile.h"

void ThrownItemRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const ThrowableProjectile *projectile = (const ThrowableProjectile *) entity;
    const Icon &icon                      = projectile->GetItem()->GetIcon();

    const Camera *camera   = Everstead::GetInstance()->GetCamera();
    glm::dvec3 front       = glm::normalize(camera->GetFront());
    glm::dvec3 worldUp     = fabs(front.y) > 0.99 ? glm::dvec3(0.0, 0.0, 1.0) : glm::dvec3(0.0, 1.0, 0.0);
    glm::dvec3 right       = glm::normalize(glm::cross(worldUp, front)) * 0.25;
    glm::dvec3 up          = glm::normalize(glm::cross(front, right)) * 0.25;
    glm::dvec3 center      = projectile->GetRenderPosition(partialTicks) + glm::dvec3(0.0, 0.125, 0.0);
    glm::dvec3 bottomLeft  = center - right - up;
    glm::dvec3 bottomRight = center + right - up;
    glm::dvec3 topRight    = center + right + up;
    glm::dvec3 topLeft     = center - right + up;

    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableCull();
    GlStateManager::SetAlphaCutoff(0.1f);
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(icon.GetTexture()->GetId());
    builder->Color(lightColor);
    builder->VertexUV(bottomLeft.x, bottomLeft.y, bottomLeft.z, icon.GetU1(), icon.GetV1());
    builder->VertexUV(bottomRight.x, bottomRight.y, bottomRight.z, icon.GetU0(), icon.GetV1());
    builder->VertexUV(topRight.x, topRight.y, topRight.z, icon.GetU0(), icon.GetV0());
    builder->VertexUV(bottomLeft.x, bottomLeft.y, bottomLeft.z, icon.GetU1(), icon.GetV1());
    builder->VertexUV(topRight.x, topRight.y, topRight.z, icon.GetU0(), icon.GetV0());
    builder->VertexUV(topLeft.x, topLeft.y, topLeft.z, icon.GetU1(), icon.GetV0());
    builder->End();
    GlStateManager::SetAlphaCutoff(-1.0f);
    GlStateManager::EnableCull();
    GlStateManager::DisableBlend();

    EntityRenderer::Render(entity, partialTicks, lightColor);
}

#endif
