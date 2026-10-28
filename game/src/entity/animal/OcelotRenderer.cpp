#ifndef EVERSTEAD_DEDICATED_SERVER

#include "OcelotRenderer.h"

#include <cmath>
#include <numbers>

#include "../../world/models/Model.h"
#include "../Mob.h"

#include <glm/trigonometric.hpp>

OcelotRenderer::OcelotRenderer() : QuadrupedRenderer("ocelot", 0.4f, 10.0f, 4.0f)
{}

void OcelotRenderer::PoseModel(const Mob *mob, Model *model, float partialTicks) const
{
    float limbSwing          = mob->GetLimbSwingOld() + (mob->GetLimbSwing() - mob->GetLimbSwingOld()) * partialTicks;
    float limbAmount         = mob->GetLimbSwingAmountOld() + (mob->GetLimbSwingAmount() - mob->GetLimbSwingAmountOld()) * partialTicks;
    float phase              = limbSwing * 0.6662f;
    const float kLegPhase[4] = {std::numbers::pi_v<float>, 0.0f, 0.0f, std::numbers::pi_v<float>};
    const char *kLegNames[4] = {"leg0", "leg1", "leg2", "leg3"};
    for (int i = 0; i < 4; i++)
    {
        model->FindPart(kLegNames[i])->SetRotation(glm::dvec3(glm::degrees(cosf(phase + kLegPhase[i]) * limbAmount), 0.0, 0.0));
    }

    model->FindPart("tail1")->SetRotation(
            glm::dvec3(glm::degrees(0.55f * std::numbers::pi_v<float> + 0.25f * std::numbers::pi_v<float> * cosf(limbSwing) * limbAmount), 0.0, 0.0));
    model->FindPart("head")->SetRotation(
            glm::dvec3(-mob->GetRenderPitch(partialTicks), mob->GetRenderBodyYaw(partialTicks) - mob->GetRenderHeadYaw(partialTicks), 0.0));
}

#endif
