#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SheepRenderer.h"

#include "../../world/models/ModelRegistry.h"
#include "Sheep.h"

#include <glm/trigonometric.hpp>

SheepRenderer::SheepRenderer() : QuadrupedRenderer("sheep", 0.7f, 8.0f, 4.0f)
{}

void SheepRenderer::PoseModel(const Mob *mob, Model *model, float partialTicks) const
{
    QuadrupedRenderer::PoseModel(mob, model, partialTicks);
    const Sheep *sheep = (const Sheep *) mob;
    float eatPosition  = sheep->GetHeadEatPositionScale(partialTicks);
    ModelPart *head    = model->FindPart("head");
    head->SetPosition(glm::dvec3(0.0, 18.0 - eatPosition * 9.0f, 8.0));
    if (eatPosition > 0.0f)
    {
        head->SetRotation(glm::dvec3(glm::degrees(sheep->GetHeadEatAngleScale(partialTicks)), head->GetRotation().y, 0.0));
    }
}

void SheepRenderer::RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const
{
    (void) lightColor;
    const Sheep *sheep = (const Sheep *) mob;
    if (sheep->IsSheared())
    {
        return;
    }

    const float kFurColors[16][3] = {{1.0f, 1.0f, 1.0f}, {0.85f, 0.5f, 0.2f}, {0.7f, 0.3f, 0.85f},  {0.4f, 0.6f, 0.85f},
                                     {0.9f, 0.9f, 0.2f}, {0.5f, 0.8f, 0.1f},  {0.95f, 0.5f, 0.65f}, {0.3f, 0.3f, 0.3f},
                                     {0.6f, 0.6f, 0.6f}, {0.3f, 0.5f, 0.65f}, {0.5f, 0.25f, 0.7f},  {0.2f, 0.3f, 0.7f},
                                     {0.4f, 0.3f, 0.2f}, {0.4f, 0.5f, 0.2f},  {0.6f, 0.2f, 0.2f},   {0.1f, 0.1f, 0.1f}};
    const float *fur              = kFurColors[sheep->GetColor()];
    uint32_t red                  = (uint32_t) (((color >> 16) & 0xFF) * fur[0]);
    uint32_t green                = (uint32_t) (((color >> 8) & 0xFF) * fur[1]);
    uint32_t blue                 = (uint32_t) ((color & 0xFF) * fur[2]);
    Model *model                  = ModelRegistry::GetInstance()->GetModel("sheep_fur");
    PoseModel(mob, model, partialTicks);
    RenderModel(mob, model, (color & 0xFF000000) | (red << 16) | (green << 8) | blue, matrix);
}

#endif
