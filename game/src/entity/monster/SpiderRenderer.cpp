#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SpiderRenderer.h"

#include <cmath>
#include <numbers>

#include "../../rendering/GlStateManager.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/ModelRegistry.h"
#include "../Mob.h"

#include <glm/trigonometric.hpp>

SpiderRenderer::SpiderRenderer() : QuadrupedRenderer("spider", 1.0f, 0.0f, 0.0f)
{}

void SpiderRenderer::PoseModel(const Mob *mob, Model *model, float partialTicks) const
{
    const float pi   = std::numbers::pi_v<float>;
    float limbSwing  = mob->GetLimbSwingOld() + (mob->GetLimbSwing() - mob->GetLimbSwingOld()) * partialTicks;
    float limbAmount = mob->GetLimbSwingAmountOld() + (mob->GetLimbSwingAmount() - mob->GetLimbSwingAmountOld()) * partialTicks;
    model->FindPart("head")->SetRotation(
            glm::dvec3(-mob->GetRenderPitch(partialTicks), mob->GetRenderBodyYaw(partialTicks) - mob->GetRenderHeadYaw(partialTicks), 0.0));

    const float kSpread[4]      = {pi / 4.0f, pi / 4.0f * 0.74f, pi / 4.0f * 0.74f, pi / 4.0f};
    const float kYaw[4]         = {pi / 4.0f, pi / 8.0f, -pi / 8.0f, -pi / 4.0f};
    const float kPhaseOffset[4] = {0.0f, pi, pi / 2.0f, pi * 3.0f / 2.0f};
    for (int i = 0; i < 4; i++)
    {
        float yaw  = kYaw[i] - cosf(limbSwing * 0.6662f * 2.0f + kPhaseOffset[i]) * 0.4f * limbAmount;
        float roll = -kSpread[i] + fabsf(sinf(limbSwing * 0.6662f + kPhaseOffset[i]) * 0.4f) * limbAmount;
        model->FindPart("leg" + std::to_string(i * 2))->SetRotation(glm::dvec3(0.0, glm::degrees(-yaw), glm::degrees(-roll)));
        model->FindPart("leg" + std::to_string(i * 2 + 1))->SetRotation(glm::dvec3(0.0, glm::degrees(yaw), glm::degrees(roll)));
    }
}

void SpiderRenderer::RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const
{
    (void) lightColor;
    Model *model         = ModelRegistry::GetInstance()->GetModel("spider_eyes");
    uint32_t entityLight = Lighting::GetEntityLight();
    bool blend           = GlStateManager::IsBlendEnabled();
    Lighting::SetEntityLight(0xFFFFFFFF);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_One, eBlendFactor_One);
    PoseModel(mob, model, partialTicks);
    RenderModel(mob, model, color | 0x00FFFFFF, matrix);
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    if (!blend)
    {
        GlStateManager::DisableBlend();
    }

    Lighting::SetEntityLight(entityLight);
}

#endif
