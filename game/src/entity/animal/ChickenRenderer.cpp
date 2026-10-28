#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChickenRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../rendering/GlStateManager.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/ModelRegistry.h"
#include "Chicken.h"

#include <glm/gtc/matrix_transform.hpp>

ChickenRenderer::ChickenRenderer()
{
    m_shadowRadius = 0.3f;
}

void ChickenRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const Chicken *chicken = (const Chicken *) entity;
    if (chicken->GetDeathTime() >= 20)
    {
        return;
    }

    glm::dvec3 pos    = entity->GetRenderPosition(partialTicks);
    float bodyYaw     = chicken->GetRenderBodyYaw(partialTicks);
    glm::dmat4 matrix = glm::translate(glm::dmat4(1.0), pos);
    matrix            = glm::rotate(matrix, (90.0 - bodyYaw) * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x, pos.y, pos.z);
    GlStateManager::Rotatef(90.0f - bodyYaw, 0.0f, 1.0f, 0.0f);
    if (chicken->GetDeathTime() > 0)
    {
        float fall = std::min(1.0f, sqrtf(((float) chicken->GetDeathTime() + partialTicks - 1.0f) / 20.0f * 1.6f));
        matrix     = glm::rotate(matrix, -fall * std::numbers::pi / 2.0, glm::dvec3(0.0, 0.0, 1.0));
        GlStateManager::Rotatef(-fall * 90.0f, 0.0f, 0.0f, 1.0f);
    }

    const float scale = 1.0f / 16.0f;
    matrix            = glm::scale(matrix, glm::dvec3(scale, scale, scale));
    GlStateManager::Scalef(scale, scale, scale);

    Model *model     = ModelRegistry::GetInstance()->GetModel("chicken");
    float limbSwing  = chicken->GetLimbSwingOld() + (chicken->GetLimbSwing() - chicken->GetLimbSwingOld()) * partialTicks;
    float limbAmount = chicken->GetLimbSwingAmountOld() + (chicken->GetLimbSwingAmount() - chicken->GetLimbSwingAmountOld()) * partialTicks;
    float legSwing   = glm::degrees(cosf(limbSwing * 0.6662f) * 1.4f * limbAmount);
    model->FindPart("leg0")->SetRotation(glm::dvec3(legSwing, 0.0, 0.0));
    model->FindPart("leg1")->SetRotation(glm::dvec3(-legSwing, 0.0, 0.0));

    float bob = glm::degrees((sinf(chicken->GetFlap(partialTicks)) + 1.0f) * chicken->GetFlapSpeed(partialTicks));
    model->FindPart("wing0")->SetRotation(glm::dvec3(0.0, 0.0, -bob));
    model->FindPart("wing1")->SetRotation(glm::dvec3(0.0, 0.0, bob));

    ModelPart *head = model->FindPart("head");
    head->SetRotation(glm::dvec3(-entity->GetRenderPitch(partialTicks), bodyYaw - chicken->GetRenderHeadYaw(partialTicks), 0.0));

    uint32_t color = Lighting::IsEnabled() ? 0xFFFFFFFF : lightColor;
    if (chicken->GetHurtTime() > 0 || chicken->GetDeathTime() > 0)
    {
        GlStateManager::SetOverlayColor(0x66FF0000);
    }

    TextureRepository *textures = ModelRegistry::GetInstance()->GetTextures();
    float alphaCutoff           = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.1f);
    glm::dmat4 headMatrix = matrix;
    glm::dmat4 bodyMatrix = matrix;
    GlStateManager::PushMatrix();
    if (chicken->IsBaby())
    {
        headMatrix = glm::translate(headMatrix, glm::dvec3(0.0, -5.0, -2.0));
        GlStateManager::Translatef(0.0f, -5.0f, -2.0f);
    }

    model->Render(*textures, color, headMatrix, head);
    GlStateManager::PopMatrix();
    GlStateManager::PushMatrix();
    if (chicken->IsBaby())
    {
        bodyMatrix = glm::scale(bodyMatrix, glm::dvec3(0.5, 0.5, 0.5));
        GlStateManager::Scalef(0.5f, 0.5f, 0.5f);
    }

    const char *kPartNames[5] = {"body", "leg0", "leg1", "wing0", "wing1"};
    for (int i = 0; i < 5; i++)
    {
        model->Render(*textures, color, bodyMatrix, model->FindPart(kPartNames[i]));
    }

    GlStateManager::PopMatrix();
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
