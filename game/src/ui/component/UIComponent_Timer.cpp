#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Timer.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/Time.h"
#include "../Screen.h"

const float UIComponent_Timer::s_animationSpeed = 3.0f;
const float UIComponent_Timer::s_iconSpacing    = 24.0f;
const float UIComponent_Timer::s_alphaFalloff   = 0.4f;

UIComponent_Timer::UIComponent_Timer(float x, float y, float size) : UIComponent()
{
    m_region = UITextureAtlas::GetRegion("assets/texture/ui/component/timer.png");
    m_x      = x;
    m_y      = y;
    m_size   = size;
    m_time   = 0.0f;
}

void UIComponent_Timer::Render()
{
    static const int ringOffsets[8][2] = {{-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}};

    Everstead *everstead = Everstead::GetInstance();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();

    m_time += (float) Time::GetDelta();

    const float halfSize = m_size * 0.5f;
    const float head     = m_time * s_animationSpeed;

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());

    for (int i = 0; i < 8; i++)
    {
        float lead = head - (float) i;
        lead       = lead - floorf(lead / 8.0f) * 8.0f;

        float vx = m_x + (float) ringOffsets[i][0] * s_iconSpacing;
        float vy = m_y + (float) ringOffsets[i][1] * s_iconSpacing;
        float x0 = round(Screen::ToActualX(vx - halfSize, actualWidth, actualHeight));
        float y0 = round(Screen::ToActualY(vy - halfSize, actualWidth, actualHeight));
        float x1 = round(Screen::ToActualX(vx + halfSize, actualWidth, actualHeight));
        float y1 = round(Screen::ToActualY(vy + halfSize, actualWidth, actualHeight));

        builder->Color(1.0f, 1.0f, 1.0f, std::max(0.0f, 1.0f - lead * s_alphaFalloff));
        builder->VertexUV(x0, y0, 0.0f, m_region.u0, m_region.v0);
        builder->VertexUV(x1, y0, 0.0f, m_region.u1, m_region.v0);
        builder->VertexUV(x1, y1, 0.0f, m_region.u1, m_region.v1);
        builder->VertexUV(x0, y0, 0.0f, m_region.u0, m_region.v0);
        builder->VertexUV(x1, y1, 0.0f, m_region.u1, m_region.v1);
        builder->VertexUV(x0, y1, 0.0f, m_region.u0, m_region.v1);
    }

    builder->End();
    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

#endif
