#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_ScrollArrow.h"

#include <algorithm>

#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/Time.h"

#include <glm/common.hpp>

UIComponent_ScrollArrow::UIComponent_ScrollArrow(eDIRECTION direction) : UIComponent()
{
    m_region           = UITextureAtlas::GetRegion(direction == eDirection_Up ? "assets/texture/ui/control/scroll_up.png"
                                                                              : "assets/texture/ui/control/scroll_down.png");
    m_x                = 0.0f;
    m_y                = 0.0f;
    m_width            = 0.0f;
    m_height           = 0.0f;
    m_flashing         = false;
    m_flashStartedAtMs = 0;
}

UIComponent_ScrollArrow::~UIComponent_ScrollArrow()
{}

void UIComponent_ScrollArrow::SetBounds(float x, float y, float width, float height)
{
    m_x      = x;
    m_y      = y;
    m_width  = width;
    m_height = height;
}

float UIComponent_ScrollArrow::GetWidthForHeight(float height) const
{
    return height * (float) m_region.pixelWidth / (float) m_region.pixelHeight;
}

float UIComponent_ScrollArrow::GetHeightForWidth(float width) const
{
    return width * (float) m_region.pixelHeight / (float) m_region.pixelWidth;
}

void UIComponent_ScrollArrow::Flash()
{
    m_flashing         = true;
    m_flashStartedAtMs = Time::GetMilliseconds();
}

void UIComponent_ScrollArrow::StopFlash()
{
    m_flashing = false;
}

float UIComponent_ScrollArrow::GetAlpha() const
{
    uint32_t flashDurationMs = 260;
    float flashOutRatio      = 0.5f;
    uint32_t elapsedMs       = Time::GetMilliseconds() - m_flashStartedAtMs;
    if (!m_flashing || elapsedMs >= flashDurationMs)
    {
        return 1.0f;
    }

    float t = (float) elapsedMs / (float) flashDurationMs;
    if (t < flashOutRatio)
    {
        return glm::mix(1.0f, 0.0f, t / flashOutRatio);
    }

    return glm::mix(0.0f, 1.0f, (t - flashOutRatio) / (1.0f - flashOutRatio));
}

void UIComponent_ScrollArrow::Render()
{
    if (!IsVisible() || m_width <= 0.0f || m_height <= 0.0f)
    {
        return;
    }

    float x1       = m_x + m_width;
    float y1       = m_y + m_height;
    float u0       = UITextureAtlas::GetU(m_region, 0.0f);
    float u1       = UITextureAtlas::GetU(m_region, 1.0f);
    float v0       = UITextureAtlas::GetV(m_region, 0.0f);
    float v1       = UITextureAtlas::GetV(m_region, 1.0f);
    uint32_t color = ((uint32_t) (std::clamp(GetAlpha(), 0.0f, 1.0f) * 255.0f + 0.5f) << 24) | 0x00FFFFFF;

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetAlphaCutoff(0.01f);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    builder->Color(color);
    builder->VertexUV(m_x, m_y, 0.0f, u0, v0);
    builder->VertexUV(x1, m_y, 0.0f, u1, v0);
    builder->VertexUV(x1, y1, 0.0f, u1, v1);
    builder->VertexUV(m_x, m_y, 0.0f, u0, v0);
    builder->VertexUV(x1, y1, 0.0f, u1, v1);
    builder->VertexUV(m_x, y1, 0.0f, u0, v1);
    builder->End();
    Textures::Unbind();

    GlStateManager::SetAlphaCutoff(-1.0f);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
}

#endif
