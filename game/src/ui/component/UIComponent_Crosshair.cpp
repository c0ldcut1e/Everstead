#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Crosshair.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../Screen.h"

UIComponent_Crosshair::UIComponent_Crosshair() : UIComponent()
{
    m_region = UITextureAtlas::GetRegion("assets/texture/ui/hud/crosshair.png");
}

void UIComponent_Crosshair::Render()
{
    Everstead *everstead     = Everstead::GetInstance();
    LocalPlayer *localPlayer = everstead->GetLocalPlayer();
    if (!everstead->options.displayHud || everstead->IsWorldMenuOpen() || (localPlayer && GameType::IsSpectator(localPlayer->GetGameType())))
    {
        return;
    }

    int actualWidth  = everstead->GetWidth();
    int actualHeight = everstead->GetHeight();
    float uiScale    = Screen::GetGuiPixelSize(everstead->options.GetHudScale(), actualWidth, actualHeight);
    float halfWidth  = round((float) m_region.pixelWidth * uiScale) * 0.5f;
    float halfHeight = round((float) m_region.pixelHeight * uiScale) * 0.5f;

    float centerX = Screen::ToActualX(Screen::WIDTH * 0.5f, actualWidth, actualHeight);
    float centerY = Screen::ToActualY(Screen::HEIGHT * 0.5f, actualWidth, actualHeight);
    float x0      = round(centerX - halfWidth);
    float y0      = round(centerY - halfHeight);
    float x1      = round(centerX + halfWidth);
    float y1      = round(centerY + halfHeight);

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());

    builder->Color(0xFFFFFFFF);
    builder->VertexUV(x0, y0, 0.0f, m_region.u0, m_region.v0);
    builder->VertexUV(x1, y0, 0.0f, m_region.u1, m_region.v0);
    builder->VertexUV(x1, y1, 0.0f, m_region.u1, m_region.v1);
    builder->VertexUV(x0, y0, 0.0f, m_region.u0, m_region.v0);
    builder->VertexUV(x1, y1, 0.0f, m_region.u1, m_region.v1);
    builder->VertexUV(x0, y1, 0.0f, m_region.u0, m_region.v1);
    builder->End();

    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

#endif
