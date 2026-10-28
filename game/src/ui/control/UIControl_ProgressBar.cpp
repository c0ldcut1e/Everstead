#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_ProgressBar.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/math/Mth.h"

const float UIControl_ProgressBar::s_progressBarDefaultWidth       = 900.0f;
const float UIControl_ProgressBar::s_progressBarDefaultHeight      = 40.0f;
const float UIControl_ProgressBar::s_progressBarSourceBorderPixels = 2.0f;

void UIControl_ProgressBar::EmitProgressBarQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1,
                                                float v1, uint32_t color, const UITextureAtlas::Region &region)
{
    if (x1 <= x0 || y1 <= y0)
    {
        return;
    }

    builder->Color(color);
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y0, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y1, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v1));
}

void UIControl_ProgressBar::EmitProgressBarNineSlice(BufferBuilder *builder, float x, float y, float width, float height, float textureWidth,
                                                     float textureHeight, float sourceBorderX, float sourceBorderY, float destinationBorderX,
                                                     float destinationBorderY, uint32_t color, const UITextureAtlas::Region &region)
{
    if (width <= 0.0f || height <= 0.0f || textureWidth <= 0.0f || textureHeight <= 0.0f)
    {
        return;
    }

    float left   = std::min(destinationBorderX, width * 0.5f);
    float right  = std::min(destinationBorderX, width - left);
    float top    = std::min(destinationBorderY, height * 0.5f);
    float bottom = std::min(destinationBorderY, height - top);

    float x0 = x;
    float x1 = x + left;
    float x2 = x + width - right;
    float x3 = x + width;

    float y0 = y;
    float y1 = y + top;
    float y2 = y + height - bottom;
    float y3 = y + height;

    float u0 = 0.0f;
    float u1 = sourceBorderX / textureWidth;
    float u2 = (textureWidth - sourceBorderX) / textureWidth;
    float u3 = 1.0f;

    float v0 = 0.0f;
    float v1 = sourceBorderY / textureHeight;
    float v2 = (textureHeight - sourceBorderY) / textureHeight;
    float v3 = 1.0f;

    EmitProgressBarQuad(builder, x0, y0, x1, y1, u0, v0, u1, v1, color, region);
    EmitProgressBarQuad(builder, x1, y0, x2, y1, u1, v0, u2, v1, color, region);
    EmitProgressBarQuad(builder, x2, y0, x3, y1, u2, v0, u3, v1, color, region);
    EmitProgressBarQuad(builder, x0, y1, x1, y2, u0, v1, u1, v2, color, region);
    EmitProgressBarQuad(builder, x1, y1, x2, y2, u1, v1, u2, v2, color, region);
    EmitProgressBarQuad(builder, x2, y1, x3, y2, u2, v1, u3, v2, color, region);
    EmitProgressBarQuad(builder, x0, y2, x1, y3, u0, v2, u1, v3, color, region);
    EmitProgressBarQuad(builder, x1, y2, x2, y3, u1, v2, u2, v3, color, region);
    EmitProgressBarQuad(builder, x2, y2, x3, y3, u2, v2, u3, v3, color, region);
}

UIControl_ProgressBar::UIControl_ProgressBar(float x, float y, float width, float height, float progress) : UIControl(x, y, width, height)
{
    m_progress         = 0.0f;
    m_preferredWidth   = std::max(width, 0.0f);
    m_preferredHeight  = std::max(height, 0.0f);
    m_backgroundRegion = UITextureAtlas::GetRegion("assets/texture/ui/control/progressbar_background.png");
    m_fillerRegion     = UITextureAtlas::GetRegion("assets/texture/ui/control/progressbar_filler.png");

    SetProgress(progress);
}

UIControl_ProgressBar::~UIControl_ProgressBar()
{}

void UIControl_ProgressBar::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    int actualWidth   = everstead->GetWidth();
    int actualHeight  = everstead->GetHeight();
    Screen::Rect rect = GetActualRect(actualWidth, actualHeight);

    float roundedX      = round(rect.x);
    float roundedY      = round(rect.y);
    float roundedWidth  = round(rect.width);
    float roundedHeight = round(rect.height);
    if (roundedWidth <= 0.0f || roundedHeight <= 0.0f)
    {
        return;
    }

    float backgroundPixelHeight = std::max(1.0f, (float) std::max(m_backgroundRegion.pixelHeight, 1));
    float backgroundPixelScale  = std::max(1.0f, round(roundedHeight / backgroundPixelHeight));
    float backgroundBorderSize  = backgroundPixelScale * s_progressBarSourceBorderPixels;

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    GlStateManager::SetAlphaCutoff(0.01f);
    EmitProgressBarNineSlice(builder, roundedX, roundedY, roundedWidth, roundedHeight, (float) std::max(m_backgroundRegion.pixelWidth, 1),
                             backgroundPixelHeight, s_progressBarSourceBorderPixels, s_progressBarSourceBorderPixels, backgroundBorderSize,
                             backgroundBorderSize, 0xFFFFFFFF, m_backgroundRegion);
    builder->End();
    Textures::Unbind();

    float fillerX      = roundedX + backgroundBorderSize;
    float fillerY      = roundedY + backgroundBorderSize;
    float fillerWidth  = std::max(0.0f, roundedWidth - backgroundBorderSize * 2.0f);
    float fillerHeight = std::max(0.0f, roundedHeight - backgroundBorderSize * 2.0f);
    float filledWidth  = round(fillerWidth * m_progress);

    if (m_progress > 0.0f && filledWidth <= 0.0f)
    {
        filledWidth = 1.0f;
    }

    if (filledWidth > 0.0f && fillerHeight > 0.0f)
    {
        builder->Begin(ePrimitiveType_Triangles);
        Textures::Bind(UITextureAtlas::GetTexture()->GetId());
        GlStateManager::SetAlphaCutoff(0.01f);
        EmitProgressBarQuad(builder, fillerX, fillerY, fillerX + filledWidth, fillerY + fillerHeight, 0.0f, 0.0f,
                            fillerWidth > 0.0f ? filledWidth / fillerWidth : 0.0f, 1.0f, 0xFFFFFFFF, m_fillerRegion);
        builder->End();
        Textures::Unbind();
    }

    GlStateManager::SetAlphaCutoff(-1.0f);

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

float UIControl_ProgressBar::GetPreferredWidth() const
{
    if (m_preferredWidth > 0.0f)
    {
        return m_preferredWidth;
    }

    return s_progressBarDefaultWidth;
}

float UIControl_ProgressBar::GetPreferredHeight() const
{
    if (m_preferredHeight > 0.0f)
    {
        return m_preferredHeight;
    }

    return s_progressBarDefaultHeight;
}

void UIControl_ProgressBar::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_ProgressBar::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

bool UIControl_ProgressBar::IsSelectable() const
{
    return false;
}

bool UIControl_ProgressBar::SetProgress(float progress)
{
    float clampedProgress = std::clamp(progress, 0.0f, 1.0f);
    if (fabsf(m_progress - clampedProgress) < 0.0001f)
    {
        return false;
    }

    m_progress = clampedProgress;
    return true;
}

float UIControl_ProgressBar::GetProgress() const
{
    return m_progress;
}

#endif
