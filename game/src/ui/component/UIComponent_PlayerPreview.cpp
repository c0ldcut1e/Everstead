#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_PlayerPreview.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../entity/EntityRendererRegistry.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../entity/player/PlayerRenderer.h"
#include "../../entity/player/RemotePlayer.h"
#include "../../rendering/GlStateManager.h"
#include "../Screen.h"

UIComponent_PlayerPreview::UIComponent_PlayerPreview() : UIComponent()
{
    m_x      = 0.0f;
    m_y      = 0.0f;
    m_width  = 0.0f;
    m_height = 0.0f;
    m_lookX  = 0.0f;
    m_lookY  = 0.0f;
}

void UIComponent_PlayerPreview::SetLookTarget(float actualX, float actualY)
{
    m_lookX = actualX;
    m_lookY = actualY;
}

UIComponent_PlayerPreview::~UIComponent_PlayerPreview()
{}

void UIComponent_PlayerPreview::Render()
{
    if (!IsVisible())
    {
        return;
    }

    const float blockPixels = 30.0f;
    const float lookPixels  = 40.0f;

    Everstead *everstead   = Everstead::GetInstance();
    int actualWidth        = everstead->GetWidth();
    int actualHeight       = everstead->GetHeight();
    float partialTicks     = everstead->GetRenderPartialTicks();
    RemotePlayer *player   = everstead->GetLocalPlayer()->GetRenderPlayer();
    PlayerRenderer *render = (PlayerRenderer *) EntityRendererRegistry::GetInstance()->GetValue(player->GetType());
    Screen::Rect rect      = Screen::ToActualRect(m_x, m_y, m_width, m_height, actualWidth, actualHeight);
    float pixel            = rect.height / 72.0f;
    float centerX          = rect.x + 25.0f * pixel;
    float feetY            = rect.y + 67.0f * pixel;

    float lookX = atanf((m_lookX - centerX) / (lookPixels * pixel));
    float lookY = atanf((rect.y + 17.0f * pixel - m_lookY) / (lookPixels * pixel));

    glm::dvec3 pos = player->GetRenderPosition(partialTicks);
    float bodyYaw  = player->GetRenderBodyYaw(partialTicks);
    player->SetInstantHeadRotation(bodyYaw - lookX * 20.0f, lookY * 20.0f);

    GlStateManager::MatrixMode(1);
    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::Ortho(0.0f, (float) actualWidth, (float) actualHeight, 0.0f, -1000.0f, 1000.0f);
    GlStateManager::MatrixMode(0);
    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::Translatef(centerX, feetY, 0.0f);
    GlStateManager::Scalef(blockPixels * pixel, -blockPixels * pixel, blockPixels * pixel);
    GlStateManager::Rotatef(-lookY * 20.0f, 1.0f, 0.0f, 0.0f);
    GlStateManager::Rotatef(lookX * 20.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(bodyYaw - 90.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Translated(-pos.x, -pos.y, -pos.z);

    GlStateManager::ClearDepth();
    GlStateManager::EnableDepthTest();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableCull();
    render->SetInGui(true);
    render->SetRenderBoundingBox(false);
    render->Render(player, partialTicks, 0xFFFFFFFF);
    render->SetInGui(false);
    GlStateManager::DisableDepthTest();

    GlStateManager::PopMatrix();
    GlStateManager::MatrixMode(1);
    GlStateManager::PopMatrix();
    GlStateManager::MatrixMode(0);
}

void UIComponent_PlayerPreview::SetBounds(float x, float y, float width, float height)
{
    m_x      = x;
    m_y      = y;
    m_width  = width;
    m_height = height;
}

#endif
