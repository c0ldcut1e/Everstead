#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SquidRenderer.h"

#include <numbers>
#include <string>

#include "../../rendering/GlStateManager.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/ModelRegistry.h"
#include "Squid.h"

#include <glm/gtc/matrix_transform.hpp>

SquidRenderer::SquidRenderer()
{
    m_shadowRadius = 0.7f;
}

void SquidRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const Squid *squid = (const Squid *) entity;
    if (squid->GetDeathTime() >= 20)
    {
        return;
    }

    glm::dvec3 pos    = entity->GetRenderPosition(partialTicks);
    float bodyYaw     = squid->GetRenderBodyYaw(partialTicks);
    float xBodyRot    = squid->GetXBodyRot(partialTicks);
    float zBodyRot    = squid->GetZBodyRot(partialTicks);
    glm::dmat4 matrix = glm::translate(glm::dmat4(1.0), pos + glm::dvec3(0.0, 0.5, 0.0));
    matrix            = glm::rotate(matrix, (90.0 - bodyYaw) * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    matrix            = glm::rotate(matrix, -xBodyRot * std::numbers::pi / 180.0, glm::dvec3(1.0, 0.0, 0.0));
    matrix            = glm::rotate(matrix, zBodyRot * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    matrix            = glm::translate(matrix, glm::dvec3(0.0, -1.2, 0.0));
    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x, pos.y + 0.5, pos.z);
    GlStateManager::Rotatef(90.0f - bodyYaw, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(-xBodyRot, 1.0f, 0.0f, 0.0f);
    GlStateManager::Rotatef(zBodyRot, 0.0f, 1.0f, 0.0f);
    GlStateManager::Translatef(0.0f, -1.2f, 0.0f);

    const float scale = 1.0f / 16.0f;
    matrix            = glm::scale(matrix, glm::dvec3(scale, scale, scale));
    GlStateManager::Scalef(scale, scale, scale);

    Model *model         = ModelRegistry::GetInstance()->GetModel("squid");
    double tentacleAngle = glm::degrees(squid->GetTentacleAngle(partialTicks));
    for (int i = 0; i < 8; i++)
    {
        model->FindPart("tentacle" + std::to_string(i))->SetRotation(glm::dvec3(tentacleAngle, i * 45.0 - 90.0, 0.0));
    }

    uint32_t color = Lighting::IsEnabled() ? 0xFFFFFFFF : lightColor;
    if (squid->GetHurtTime() > 0 || squid->GetDeathTime() > 0)
    {
        GlStateManager::SetOverlayColor(0x66FF0000);
    }

    TextureRepository *textures = ModelRegistry::GetInstance()->GetTextures();
    float alphaCutoff           = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.1f);
    model->Render(*textures, color, matrix);
    GlStateManager::SetAlphaCutoff(alphaCutoff);
    GlStateManager::SetOverlayColor(0);
    GlStateManager::PopMatrix();

    RenderFire(entity, partialTicks);
    if (m_renderBoundingBox)
    {
        RenderBoundingBox(entity, partialTicks);
    }
}

#endif
