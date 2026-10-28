#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SlimeRenderer.h"

#include "../../rendering/GlStateManager.h"
#include "../../world/models/ModelRegistry.h"
#include "Slime.h"

#include <glm/gtc/matrix_transform.hpp>

SlimeRenderer::SlimeRenderer() : QuadrupedRenderer("slime", 0.25f, 0.0f, 0.0f)
{}

void SlimeRenderer::ApplyTransform(const Mob *mob, float partialTicks, glm::dmat4 *matrix) const
{
    const Slime *slime = (const Slime *) mob;
    float size         = (float) slime->GetSlimeSize();
    float squish       = slime->GetSquish(partialTicks) / (size * 0.5f + 1.0f);
    float width        = 1.0f / (squish + 1.0f);
    *matrix            = glm::scale(*matrix, glm::dvec3(width * size, size / width, width * size));
    GlStateManager::Scalef(width * size, size / width, width * size);
}

void SlimeRenderer::PoseModel(const Mob *mob, Model *model, float partialTicks) const
{
    (void) mob;
    (void) model;
    (void) partialTicks;
}

void SlimeRenderer::RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const
{
    (void) lightColor;
    (void) partialTicks;
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    RenderModel(mob, ModelRegistry::GetInstance()->GetModel("slime_outer"), color, matrix);
    GlStateManager::DisableBlend();
}

#endif
