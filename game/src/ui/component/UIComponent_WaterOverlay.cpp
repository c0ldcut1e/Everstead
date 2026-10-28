#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_WaterOverlay.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../../scene/Camera.h"
#include "../../world/Level.h"
#include "../../world/block/BlockPos.h"

const float UIComponent_WaterOverlay::s_tileCount     = 4.0f;
const float UIComponent_WaterOverlay::s_scrollDivisor = 64.0f;
const float UIComponent_WaterOverlay::s_opacity       = 0.1f;

UIComponent_WaterOverlay::UIComponent_WaterOverlay() : UIComponent()
{
    m_texture = TextureRepository::GetInstance()->GetRaw("assets/texture/misc/underwater.png");
    m_scroll  = 0.0f;
    m_lastYaw = 0.0f;
}

UIComponent_WaterOverlay::~UIComponent_WaterOverlay()
{}

void UIComponent_WaterOverlay::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    LocalPlayer *player  = everstead->GetLocalPlayer();
    Level *level         = player ? player->GetLevel() : nullptr;
    if (!level)
    {
        return;
    }

    glm::dvec3 cameraPosition = everstead->GetCamera()->GetPosition();
    if (!level->IsLiquidAt(cameraPosition))
    {
        return;
    }

    BlockPos cameraBlock((int) floor(cameraPosition.x), (int) floor(cameraPosition.y), (int) floor(cameraPosition.z));
    float light =
            (float) std::max(level->GetSkyLightLevel(cameraBlock) - level->GetSkyDarken(), (int) level->GetBlockLightLevel(cameraBlock)) / 15.0f;
    float brightness = light / (4.0f - 3.0f * light);
    float width      = (float) everstead->GetWidth();
    float height     = (float) everstead->GetHeight();
    // Yaw wraps at half a turn, so we scroll by how far it moved instead of where it is
    m_scroll  = fmodf(m_scroll - remainderf(player->GetYaw() - m_lastYaw, 360.0f) / s_scrollDivisor, 1.0f);
    m_lastYaw = player->GetYaw();
    float v   = player->GetPitch() / s_scrollDivisor;

    Tesselator *renderer   = Tesselator::GetInstance();
    BufferBuilder *builder = renderer->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    m_texture->SetWrapMode(eTextureWrap_Repeat, eTextureWrap_Repeat);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(m_texture->GetId());
    builder->Color(brightness, brightness, brightness, s_opacity);
    builder->VertexUV(0.0f, height, 0.0f, s_tileCount + m_scroll, s_tileCount + v);
    builder->VertexUV(width, height, 0.0f, m_scroll, s_tileCount + v);
    builder->VertexUV(width, 0.0f, 0.0f, m_scroll, v);
    builder->VertexUV(0.0f, height, 0.0f, s_tileCount + m_scroll, s_tileCount + v);
    builder->VertexUV(width, 0.0f, 0.0f, m_scroll, v);
    builder->VertexUV(0.0f, 0.0f, 0.0f, s_tileCount + m_scroll, v);
    builder->End();
    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

#endif
