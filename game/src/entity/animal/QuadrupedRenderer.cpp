#ifndef EVERSTEAD_DEDICATED_SERVER

#include "QuadrupedRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../rendering/GlStateManager.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/ModelRegistry.h"
#include "../Mob.h"

#include <glm/gtc/matrix_transform.hpp>

QuadrupedRenderer::QuadrupedRenderer(const std::string &modelName, float shadowRadius, float babyHeadOffsetY, float babyHeadOffsetZ)
{
    m_modelName       = modelName;
    m_shadowRadius    = shadowRadius;
    m_babyHeadOffsetY = babyHeadOffsetY;
    m_babyHeadOffsetZ = babyHeadOffsetZ;
}

void QuadrupedRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const Mob *mob = (const Mob *) entity;
    if (mob->GetDeathTime() >= 20)
    {
        return;
    }

    glm::dvec3 pos    = entity->GetRenderPosition(partialTicks);
    float bodyYaw     = mob->GetRenderBodyYaw(partialTicks);
    glm::dmat4 matrix = glm::translate(glm::dmat4(1.0), pos);
    matrix            = glm::rotate(matrix, (90.0 - bodyYaw) * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x, pos.y, pos.z);
    GlStateManager::Rotatef(90.0f - bodyYaw, 0.0f, 1.0f, 0.0f);
    if (mob->GetDeathTime() > 0)
    {
        float fall = std::min(1.0f, sqrtf(((float) mob->GetDeathTime() + partialTicks - 1.0f) / 20.0f * 1.6f));
        matrix     = glm::rotate(matrix, -fall * std::numbers::pi / 2.0, glm::dvec3(0.0, 0.0, 1.0));
        GlStateManager::Rotatef(-fall * 90.0f, 0.0f, 0.0f, 1.0f);
    }

    ApplyTransform(mob, partialTicks, &matrix);
    const float scale = 1.0f / 16.0f;
    matrix            = glm::scale(matrix, glm::dvec3(scale, scale, scale));
    GlStateManager::Scalef(scale, scale, scale);

    uint32_t color = Lighting::IsEnabled() ? 0xFFFFFFFF : lightColor;
    GlStateManager::SetOverlayColor(GetOverlayColor(mob, partialTicks));

    Model *model = ModelRegistry::GetInstance()->GetModel(m_modelName);
    PoseModel(mob, model, partialTicks);
    RenderModel(mob, model, color, matrix);
    RenderLayers(mob, color, lightColor, matrix, partialTicks);
    GlStateManager::SetOverlayColor(0);
    GlStateManager::PopMatrix();

    RenderFire(entity, partialTicks);
    if (m_renderBoundingBox)
    {
        RenderBoundingBox(entity, partialTicks);
    }
}

void QuadrupedRenderer::ApplyTransform(const Mob *mob, float partialTicks, glm::dmat4 *matrix) const
{
    (void) mob;
    (void) partialTicks;
    (void) matrix;
}

uint32_t QuadrupedRenderer::GetOverlayColor(const Mob *mob, float partialTicks) const
{
    (void) partialTicks;
    return mob->GetHurtTime() > 0 || mob->GetDeathTime() > 0 ? 0x66FF0000 : 0;
}

void QuadrupedRenderer::PoseModel(const Mob *mob, Model *model, float partialTicks) const
{
    float limbSwing          = mob->GetLimbSwingOld() + (mob->GetLimbSwing() - mob->GetLimbSwingOld()) * partialTicks;
    float limbAmount         = mob->GetLimbSwingAmountOld() + (mob->GetLimbSwingAmount() - mob->GetLimbSwingAmountOld()) * partialTicks;
    float phase              = limbSwing * 0.6662f;
    const float kLegPhase[4] = {0.0f, std::numbers::pi_v<float>, std::numbers::pi_v<float>, 0.0f};
    const char *kLegNames[4] = {"leg0", "leg1", "leg2", "leg3"};
    for (int i = 0; i < 4; i++)
    {
        model->FindPart(kLegNames[i])->SetRotation(glm::dvec3(glm::degrees(cosf(phase + kLegPhase[i]) * 1.4f * limbAmount), 0.0, 0.0));
    }

    model->FindPart("head")->SetRotation(
            glm::dvec3(-mob->GetRenderPitch(partialTicks), mob->GetRenderBodyYaw(partialTicks) - mob->GetRenderHeadYaw(partialTicks), 0.0));
}

void QuadrupedRenderer::RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const
{
    (void) mob;
    (void) color;
    (void) lightColor;
    (void) matrix;
    (void) partialTicks;
}

void QuadrupedRenderer::RenderModel(const Mob *mob, Model *model, uint32_t color, const glm::dmat4 &matrix) const
{
    TextureRepository *textures = ModelRegistry::GetInstance()->GetTextures();
    if (!mob->IsBaby())
    {
        model->Render(*textures, color, matrix);
        return;
    }

    glm::dmat4 headMatrix = matrix;
    glm::dmat4 bodyMatrix = matrix;
    GlStateManager::PushMatrix();
    headMatrix = glm::translate(headMatrix, glm::dvec3(0.0, -m_babyHeadOffsetY, -m_babyHeadOffsetZ));
    GlStateManager::Translatef(0.0f, -m_babyHeadOffsetY, -m_babyHeadOffsetZ);

    model->Render(*textures, color, headMatrix, model->FindPart("head"));
    GlStateManager::PopMatrix();
    GlStateManager::PushMatrix();
    bodyMatrix = glm::scale(bodyMatrix, glm::dvec3(0.5, 0.5, 0.5));
    GlStateManager::Scalef(0.5f, 0.5f, 0.5f);

    const char *kPartNames[5] = {"body", "leg0", "leg1", "leg2", "leg3"};
    for (int i = 0; i < 5; i++)
    {
        model->Render(*textures, color, bodyMatrix, model->FindPart(kPartNames[i]));
    }

    GlStateManager::PopMatrix();
}

#endif
