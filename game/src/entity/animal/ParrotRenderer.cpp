#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ParrotRenderer.h"

#include <cmath>

#include "../../rendering/GlStateManager.h"
#include "../../world/models/ModelRegistry.h"
#include "Parrot.h"

#include <glm/trigonometric.hpp>

ParrotRenderer::ParrotRenderer() : QuadrupedRenderer("parrot_red_blue", 0.3f, 0.0f, 0.0f)
{}

void ParrotRenderer::PoseModel(const Mob *mob, Model *model, float partialTicks) const
{
    (void) model;
    const Parrot *parrot = (const Parrot *) mob;
    Model *variantModel  = ModelRegistry::GetInstance()->GetModel(std::string("parrot_") + Parrot::kVariants[parrot->GetVariant()]);
    float limbSwing      = mob->GetLimbSwingOld() + (mob->GetLimbSwing() - mob->GetLimbSwingOld()) * partialTicks;
    float limbAmount     = mob->GetLimbSwingAmountOld() + (mob->GetLimbSwingAmount() - mob->GetLimbSwingAmountOld()) * partialTicks;
    float legSwing       = glm::degrees(cosf(limbSwing * 0.6662f) * 1.4f * limbAmount);
    variantModel->FindPart("leg0")->SetRotation(glm::dvec3(legSwing, 0.0, 0.0));
    variantModel->FindPart("leg1")->SetRotation(glm::dvec3(-legSwing, 0.0, 0.0));

    float wing = glm::degrees(0.0873f + (sinf(parrot->GetFlap(partialTicks)) + 1.0f) * parrot->GetFlapSpeed(partialTicks));
    variantModel->FindPart("wing0")->SetRotation(glm::dvec3(-40.0, 180.0, wing));
    variantModel->FindPart("wing1")->SetRotation(glm::dvec3(-40.0, 180.0, -wing));
    variantModel->FindPart("head")->SetRotation(
            glm::dvec3(-mob->GetRenderPitch(partialTicks), mob->GetRenderBodyYaw(partialTicks) - mob->GetRenderHeadYaw(partialTicks), 0.0));
}

void ParrotRenderer::RenderModel(const Mob *mob, Model *model, uint32_t color, const glm::dmat4 &matrix) const
{
    (void) model;
    const Parrot *parrot  = (const Parrot *) mob;
    ModelRegistry *models = ModelRegistry::GetInstance();
    float alphaCutoff     = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.1f);
    GlStateManager::DisableCull();
    models->GetModel(std::string("parrot_") + Parrot::kVariants[parrot->GetVariant()])->Render(*models->GetTextures(), color, matrix);
    GlStateManager::EnableCull();
    GlStateManager::SetAlphaCutoff(alphaCutoff);
}

#endif
