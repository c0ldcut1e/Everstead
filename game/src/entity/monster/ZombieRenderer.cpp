#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ZombieRenderer.h"

#include <cmath>
#include <numbers>

#include "../../rendering/GlStateManager.h"
#include "../../world/models/Model.h"
#include "../../world/models/ModelRegistry.h"
#include "../Mob.h"

#include <glm/gtc/matrix_transform.hpp>

ZombieRenderer::ZombieRenderer(const std::string &modelName) : QuadrupedRenderer(modelName, 0.5f, 0.0f, 0.0f)
{}

// The humanoid model has its feet two pixels below the origin
void ZombieRenderer::ApplyTransform(const Mob *mob, float partialTicks, glm::dmat4 *matrix) const
{
    (void) mob;
    (void) partialTicks;
    *matrix = glm::translate(*matrix, glm::dvec3(0.0, 0.125, 0.0));
    GlStateManager::Translatef(0.0f, 0.125f, 0.0f);
}

void ZombieRenderer::PoseModel(const Mob *mob, Model *model, float partialTicks) const
{
    const float pi   = std::numbers::pi_v<float>;
    float limbSwing  = mob->GetLimbSwingOld() + (mob->GetLimbSwing() - mob->GetLimbSwingOld()) * partialTicks;
    float limbAmount = mob->GetLimbSwingAmountOld() + (mob->GetLimbSwingAmount() - mob->GetLimbSwingAmountOld()) * partialTicks;
    float phase      = limbSwing * 0.6662f;
    float bob        = (float) mob->GetTotalTickCount() + partialTicks;
    float attackTime = mob->GetAttackAnim(partialTicks);
    float swing      = sinf(attackTime * pi);
    float raise      = sinf((1.0f - (1.0f - attackTime) * (1.0f - attackTime)) * pi);
    float armSwayX   = sinf(bob * 0.067f) * 0.05f;
    float armSwayZ   = cosf(bob * 0.09f) * 0.05f + 0.05f;
    float armPitch   = -pi / 2.0f - (swing * 1.2f - raise * 0.4f);
    float armYaw     = 0.1f - swing * 0.6f;

    model->FindPart("head")->SetRotation(
            glm::dvec3(-mob->GetRenderPitch(partialTicks), mob->GetRenderBodyYaw(partialTicks) - mob->GetRenderHeadYaw(partialTicks), 0.0));
    model->FindPart("rightArm")->SetRotation(glm::dvec3(glm::degrees(armPitch + armSwayX), glm::degrees(armYaw), glm::degrees(-armSwayZ)));
    model->FindPart("leftArm")->SetRotation(glm::dvec3(glm::degrees(armPitch - armSwayX), glm::degrees(-armYaw), glm::degrees(armSwayZ)));
    model->FindPart("rightLeg")->SetRotation(glm::dvec3(glm::degrees(cosf(phase) * 1.4f * limbAmount), 0.0, 0.0));
    model->FindPart("leftLeg")->SetRotation(glm::dvec3(glm::degrees(cosf(phase + pi) * 1.4f * limbAmount), 0.0, 0.0));
    if (mob->GetVehicle())
    {
        model->FindPart("rightArm")->SetRotation(model->FindPart("rightArm")->GetRotation() + glm::dvec3(-36.0, 0.0, 0.0));
        model->FindPart("leftArm")->SetRotation(model->FindPart("leftArm")->GetRotation() + glm::dvec3(-36.0, 0.0, 0.0));
        model->FindPart("rightLeg")->SetRotation(glm::dvec3(-72.0, -18.0, 0.0));
        model->FindPart("leftLeg")->SetRotation(glm::dvec3(-72.0, 18.0, 0.0));
    }
}

void ZombieRenderer::RenderModel(const Mob *mob, Model *model, uint32_t color, const glm::dmat4 &matrix) const
{
    TextureRepository *textures = ModelRegistry::GetInstance()->GetTextures();
    if (!mob->IsBaby())
    {
        model->Render(*textures, color, matrix);
        return;
    }

    // The baby head scales less than the body, so it is scaled around the neck and moved down onto the smaller body
    glm::dmat4 headMatrix =
            glm::translate(glm::scale(glm::translate(matrix, glm::dvec3(0.0, 10.0, 0.0)), glm::dvec3(0.75)), glm::dvec3(0.0, -22.0, 0.0));
    GlStateManager::PushMatrix();
    GlStateManager::Translatef(0.0f, 10.0f, 0.0f);
    GlStateManager::Scalef(0.75f, 0.75f, 0.75f);
    GlStateManager::Translatef(0.0f, -22.0f, 0.0f);
    model->Render(*textures, color, headMatrix, model->FindPart("head"));
    GlStateManager::PopMatrix();

    glm::dmat4 bodyMatrix =
            glm::translate(glm::scale(glm::translate(matrix, glm::dvec3(0.0, -2.0, 0.0)), glm::dvec3(0.5)), glm::dvec3(0.0, 2.0, 0.0));
    GlStateManager::PushMatrix();
    GlStateManager::Translatef(0.0f, -2.0f, 0.0f);
    GlStateManager::Scalef(0.5f, 0.5f, 0.5f);
    GlStateManager::Translatef(0.0f, 2.0f, 0.0f);
    const char *kPartNames[5] = {"body", "rightArm", "leftArm", "rightLeg", "leftLeg"};
    for (int i = 0; i < 5; i++)
    {
        model->Render(*textures, color, bodyMatrix, model->FindPart(kPartNames[i]));
    }

    GlStateManager::PopMatrix();
}

#endif
