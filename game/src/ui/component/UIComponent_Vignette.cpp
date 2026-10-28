#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Vignette.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../../utils/math/Mth.h"

UIComponent_Vignette::UIComponent_Vignette() : UIComponent()
{
    m_texture          = TextureRepository::GetInstance()->GetRaw("assets/texture/misc/vignette.png");
    m_smoothedStrength = 1.0f;
}

UIComponent_Vignette::~UIComponent_Vignette()
{}

void UIComponent_Vignette::Render()
{
    if (!m_texture)
    {
        return;
    }

    Everstead *mc       = Everstead::GetInstance();
    LocalPlayer *player = mc ? mc->GetLocalPlayer() : nullptr;
    Level *level        = player ? player->GetLevel() : nullptr;
    if (!player || !level)
    {
        m_smoothedStrength = 1.0f;
        return;
    }

    glm::dvec3 playerPos = player->GetPosition();
    glm::dvec3 eyePos    = player->GetEyePosition(mc->GetRenderPartialTicks());
    BlockPos pos((int) floor(eyePos.x), (int) floor(eyePos.y), (int) floor(eyePos.z));

    uint8_t light        = level->GetLightLevel(pos);
    float brightness     = std::clamp((float) light / 15.0f, 0.0f, 1.0f);
    float targetStrength = 1.0f - brightness;
    m_smoothedStrength += (targetStrength - m_smoothedStrength) * 0.01f;
    m_smoothedStrength = std::clamp(m_smoothedStrength, 0.0f, 1.0f);

    const WorldBorder &border = level->GetWorldBorder();
    double borderDistance     = border.GetDistanceToBorder(playerPos.x, playerPos.z);
    double movingDistance =
            std::min(border.GetLerpSpeed() * (double) border.GetWarningTime() * 20.0, fabs(border.GetLerpTarget() - border.GetSize()));
    double warningDistance = std::max((double) border.GetWarningBlocks(), movingDistance);
    float warning          = borderDistance < warningDistance ? (float) std::clamp(1.0 - borderDistance / warningDistance, 0.0, 1.0) : 0.0f;
    if (m_smoothedStrength <= 0.001f && warning <= 0.0f)
    {
        return;
    }

    if (!mc)
    {
        return;
    }

    float width  = (float) mc->GetWidth();
    float height = (float) mc->GetHeight();

    Tesselator *renderer   = Tesselator::GetInstance();
    BufferBuilder *builder = renderer->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_Zero, eBlendFactor_OneMinusSrcColor);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(m_texture->GetId());
    if (warning > 0.0f)
    {
        builder->Color(0.0f, warning, warning, 1.0f);
    }
    else
    {
        builder->Color(m_smoothedStrength, m_smoothedStrength, m_smoothedStrength, 1.0f);
    }
    builder->VertexUV(0.0f, height, 0.0f, 0.0f, 1.0f);
    builder->VertexUV(width, height, 0.0f, 1.0f, 1.0f);
    builder->VertexUV(width, 0.0f, 0.0f, 1.0f, 0.0f);
    builder->VertexUV(0.0f, height, 0.0f, 0.0f, 1.0f);
    builder->VertexUV(width, 0.0f, 0.0f, 1.0f, 0.0f);
    builder->VertexUV(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    builder->End();
    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

#endif
