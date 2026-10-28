#ifndef EVERSTEAD_DEDICATED_SERVER

#include "CreeperRenderer.h"

#include <algorithm>
#include <cmath>

#include "../../rendering/GlStateManager.h"
#include "Creeper.h"

#include <glm/gtc/matrix_transform.hpp>

CreeperRenderer::CreeperRenderer() : QuadrupedRenderer("creeper", 0.5f, 0.0f, 0.0f)
{}

void CreeperRenderer::ApplyTransform(const Mob *mob, float partialTicks, glm::dmat4 *matrix) const
{
    float swelling = ((const Creeper *) mob)->GetSwelling(partialTicks);
    float wobble   = 1.0f + sinf(swelling * 100.0f) * swelling * 0.01f;
    swelling       = std::clamp(swelling, 0.0f, 1.0f);
    swelling       = swelling * swelling * swelling * swelling;
    float width    = (1.0f + swelling * 0.4f) * wobble;
    float height   = (1.0f + swelling * 0.1f) / wobble;
    *matrix        = glm::scale(*matrix, glm::dvec3(width, height, width));
    GlStateManager::Scalef(width, height, width);
}

uint32_t CreeperRenderer::GetOverlayColor(const Mob *mob, float partialTicks) const
{
    uint32_t hurtColor = QuadrupedRenderer::GetOverlayColor(mob, partialTicks);
    float swelling     = ((const Creeper *) mob)->GetSwelling(partialTicks);
    if (hurtColor || (int) (swelling * 10.0f) % 2 == 0)
    {
        return hurtColor;
    }

    return ((uint32_t) (std::clamp(swelling * 0.2f, 0.0f, 1.0f) * 255.0f) << 24) | 0xFFFFFF;
}

#endif
