#ifndef EVERSTEAD_DEDICATED_SERVER

#include "BoatRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#include "../../rendering/GlStateManager.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/ModelRegistry.h"
#include "Boat.h"

#include <glm/gtc/matrix_transform.hpp>

BoatRenderer::BoatRenderer()
{
    m_shadowRadius = 0.5f;
}

glm::dmat4 BoatRenderer::PushTransform(const Boat *boat, float partialTicks) const
{
    glm::dvec3 pos    = boat->GetRenderPosition(partialTicks);
    float yaw         = -boat->GetRenderYaw(partialTicks);
    glm::dmat4 matrix = glm::translate(glm::dmat4(1.0), glm::dvec3(pos.x, pos.y + 0.375, pos.z));
    matrix            = glm::rotate(matrix, yaw * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x, pos.y + 0.375, pos.z);
    GlStateManager::Rotatef(yaw, 0.0f, 1.0f, 0.0f);
    float hitTime = (float) boat->GetTimeSinceHit() - partialTicks;
    float damage  = std::max(0.0f, boat->GetDamage() - partialTicks);
    if (hitTime > 0.0f)
    {
        float wobble = sinf(hitTime) * hitTime * damage / 10.0f * (float) boat->GetHurtDir();
        matrix       = glm::rotate(matrix, wobble * std::numbers::pi / 180.0, glm::dvec3(1.0, 0.0, 0.0));
        GlStateManager::Rotatef(wobble, 1.0f, 0.0f, 0.0f);
    }

    const float scale = 1.0f / 16.0f;
    matrix            = glm::scale(matrix, glm::dvec3(scale, scale, scale));
    GlStateManager::Scalef(scale, scale, scale);
    return matrix;
}

void BoatRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const Boat *boat  = (const Boat *) entity;
    glm::dmat4 matrix = PushTransform(boat, partialTicks);

    Model *model = ModelRegistry::GetInstance()->GetModel(std::string("boat_") + Boat::kWoodTypes[boat->GetWoodType()]);
    for (int side = 0; side < 2; side++)
    {
        float rowing = boat->GetRowingTime(side, partialTicks);
        float pitch  = std::lerp(-60.0f, -15.0f, (sinf(-rowing) + 1.0f) / 2.0f);
        float turn   = std::lerp(-45.0f, 45.0f, (sinf(-rowing + 1.0f) + 1.0f) / 2.0f);
        if (side == 1)
        {
            turn = 180.0f - turn;
        }

        model->FindPart(side == 0 ? "paddle0" : "paddle1")->SetRotation(glm::dvec3(pitch, -turn, -11.25));
    }

    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.1f);
    model->Render(*ModelRegistry::GetInstance()->GetTextures(), Lighting::IsEnabled() ? 0xFFFFFFFF : lightColor, matrix);
    GlStateManager::SetAlphaCutoff(alphaCutoff);
    GlStateManager::PopMatrix();

    RenderFire(entity, partialTicks);
    if (m_renderBoundingBox)
    {
        RenderBoundingBox(entity, partialTicks);
    }
}

void BoatRenderer::RenderWaterPatch(const Boat *boat, float partialTicks) const
{
    glm::dmat4 matrix = PushTransform(boat, partialTicks);
    ModelRegistry::GetInstance()->GetModel("boat_water_patch")->Render(*ModelRegistry::GetInstance()->GetTextures(), 0xFFFFFFFF, matrix);
    GlStateManager::PopMatrix();
}

#endif
