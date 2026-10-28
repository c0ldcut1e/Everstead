#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_Button.h"
#include "../../utils/Time.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../Screen.h"
#include "../UIControllerGlyphRenderer.h"

const float UIControl_Button::s_buttonDefaultWidth  = 600.0f;
const float UIControl_Button::s_buttonDefaultHeight = 50.0f;
const float UIControl_Button::s_buttonTextScale     = 0.8f;

void UIControl_Button::EmitButtonQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                                      uint32_t color, const UITextureAtlas::Region &region)
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

UIControl_Button::UIControl_Button(float x, float y, float width, float height, const std::wstring &label)
    : UIControl_Button(x, y, width, height, label, "assets/texture/ui/control/listbutton.png", "assets/texture/ui/control/listbutton_highlight.png",
                       s_buttonDefaultHeight, s_buttonDefaultWidth)
{}

UIControl_Button::UIControl_Button(float x, float y, float width, float height, const std::wstring &label, const char *normalTexturePath,
                                   const char *highlightTexturePath)
    : UIControl_Button(x, y, width, height, label, normalTexturePath, highlightTexturePath, s_buttonDefaultHeight, s_buttonDefaultWidth)
{}

UIControl_Button::~UIControl_Button()
{}

void UIControl_Button::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    Font *font = everstead->GetDefaultFont();

    int actualWidth   = everstead->GetWidth();
    int actualHeight  = everstead->GetHeight();
    Screen::Rect rect = GetActualRect(actualWidth, actualHeight);
    float uiScale     = Screen::ScaleUniform(actualWidth, actualHeight);

    float displayHeight = round(rect.height);
    float displayWidth;
    if (m_preferredWidth > 0.0f)
    {
        displayWidth = round(Screen::ToActualLength(m_preferredWidth, actualWidth, actualHeight));
    }
    else
    {
        displayWidth = round(displayHeight * ((float) m_normalRegion.pixelWidth / (float) m_normalRegion.pixelHeight));
        if (displayWidth <= 0.0f)
        {
            displayWidth = round(Screen::ToActualLength(s_buttonDefaultWidth, actualWidth, actualHeight));
        }
    }

    float btnX = round(rect.x + (rect.width - displayWidth) * 0.5f);
    float btnY = round(rect.y);

    BufferBuilder *builder               = Tesselator::GetInstance()->GetBuilder();
    const UITextureAtlas::Region &region = m_selected ? m_highlightRegion : m_normalRegion;

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    GlStateManager::SetAlphaCutoff(0.01f);
    uint32_t buttonColor = m_enabled ? 0xFFFFFFFF : 0x80FFFFFF;
    // A set texel length pins the outline to whole gui pixels like a 9-slice instead of scaling it with the button height
    float texelSize = m_texelLength > 0.0f ? round(Screen::ToActualLength(m_texelLength, actualWidth, actualHeight)) * 0.5f
                                           : displayHeight / (float) region.pixelHeight;
    float border    = std::min(round(texelSize * kBorderTexels), std::min(displayWidth, displayHeight) * 0.5f);
    float borderU   = kBorderTexels / (float) region.pixelWidth;
    float borderV   = kBorderTexels / (float) region.pixelHeight;
    float xs[]      = {btnX, btnX + border, btnX + displayWidth - border, btnX + displayWidth};
    float ys[]      = {btnY, btnY + border, btnY + displayHeight - border, btnY + displayHeight};
    float middleU   = std::min((displayWidth - border * 2.0f) / texelSize / (float) region.pixelWidth, 1.0f - borderU * 2.0f);
    float middleV   = std::min((displayHeight - border * 2.0f) / texelSize / (float) region.pixelHeight, 1.0f - borderV * 2.0f);
    float u0s[]     = {0.0f, borderU, 1.0f - borderU};
    float u1s[]     = {borderU, borderU + middleU, 1.0f};
    float v0s[]     = {0.0f, borderV, 1.0f - borderV};
    float v1s[]     = {borderV, borderV + middleV, 1.0f};
    for (int row = 0; row < 3; row++)
    {
        for (int column = 0; column < 3; column++)
        {
            EmitButtonQuad(builder, xs[column], ys[row], xs[column + 1], ys[row + 1], u0s[column], v0s[row], u1s[column], v1s[row], buttonColor,
                           region);
        }
    }
    builder->End();
    Textures::Unbind();
    GlStateManager::SetAlphaCutoff(-1.0f);

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();

    if (!m_label.empty())
    {
        float fontScale = font->SnapScale(uiScale * s_buttonTextScale);
        float textWidth = font->GetWidth(m_label, fontScale);
        float textX     = round(btnX + (displayWidth - textWidth) * 0.5f);
        if (m_horizontalAlign == eHorizontalAlign_Left)
        {
            textX = round(btnX + Screen::ToActualLength(24.0f, actualWidth, actualHeight));
        }

        float textBaselineY = round(btnY + (displayHeight - font->GetLineHeight(fontScale)) * 0.5f + font->GetAscent(fontScale));
        bool flashing       = m_wasPressed && (Time::GetMilliseconds() - m_pressedAtMs) < 500u;
        uint32_t textColor  = (flashing || !m_selected) ? s_buttonDefaultTextColor : s_buttonSelectedTextColor;
        if (!m_enabled)
        {
            textColor = 0xFF808080;
        }

        font->SetNearest(true);
        font->Draw(m_label, textX + 2.0f, textBaselineY + 2.0f, fontScale, 0xFF000000);
        font->Draw(m_label, textX, textBaselineY, fontScale, textColor);

        if (!m_rightLabel.empty())
        {
            float rightWidth = font->GetWidth(m_rightLabel, fontScale);
            float rightX     = round(btnX + displayWidth - Screen::ToActualLength(24.0f, actualWidth, actualHeight) - rightWidth);
            font->Draw(m_rightLabel, rightX + 2.0f, textBaselineY + 2.0f, fontScale, 0xFF000000);
            font->Draw(m_rightLabel, rightX, textBaselineY, fontScale, m_rightLabelColor);
        }
        else if (m_rightIcon != eControllerButton_Count)
        {
            float iconSize = round(displayHeight * kRightIconScale);
            float iconX    = round(btnX + displayWidth - Screen::ToActualLength(24.0f, actualWidth, actualHeight) -
                                   UIControllerGlyphRenderer::GetWidth(m_rightIcon, iconSize));
            UIControllerGlyphRenderer::Render(m_rightIcon, iconX, round(btnY + (displayHeight - iconSize) * 0.5f), iconSize);
        }
    }
}

float UIControl_Button::GetPreferredWidth() const
{
    if (m_preferredWidth > 0.0f)
    {
        return m_preferredWidth;
    }

    return m_preferredHeight * ((float) m_normalRegion.pixelWidth / (float) m_normalRegion.pixelHeight);
}

float UIControl_Button::GetPreferredHeight() const
{
    if (m_preferredHeight > 0.0f)
    {
        return m_preferredHeight;
    }

    return m_height;
}

void UIControl_Button::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_Button::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

void UIControl_Button::SetTexelLength(float texelLength)
{
    m_texelLength = texelLength;
}

void UIControl_Button::Activate()
{
    if (!m_enabled)
    {
        return;
    }

    m_wasPressed  = true;
    m_pressedAtMs = Time::GetMilliseconds();
}

bool UIControl_Button::OnPointerPressed(int, int, int, int)
{
    if (!m_enabled)
    {
        return false;
    }

    m_wasPressed  = true;
    m_pressedAtMs = Time::GetMilliseconds();
    return true;
}

const std::wstring &UIControl_Button::GetLabel() const
{
    return m_label;
}

void UIControl_Button::SetLabel(const std::wstring &label)
{
    m_label = label;
}

void UIControl_Button::SetHorizontalAlign(eHORIZONTAL_ALIGN align)
{
    m_horizontalAlign = align;
}

UIControl_Button::eHORIZONTAL_ALIGN UIControl_Button::GetHorizontalAlign() const
{
    return m_horizontalAlign;
}

UIControl_Button::UIControl_Button(float x, float y, float width, float height, const std::wstring &label, const char *normalTexturePath,
                                   const char *highlightTexturePath, float preferredHeight, float preferredWidth)
    : UIControl(x, y, width, height)
{
    m_label           = label;
    m_rightLabel      = L"";
    m_rightLabelColor = 0xFFB0B0B0;
    m_rightIcon       = eControllerButton_Count;
    m_normalRegion    = UITextureAtlas::GetRegion(normalTexturePath);
    m_highlightRegion = UITextureAtlas::GetRegion(highlightTexturePath);
    m_preferredHeight = std::max(preferredHeight, 0.0f);
    m_preferredWidth  = std::max(preferredWidth, 0.0f);
    m_texelLength     = 0.0f;
    m_horizontalAlign = eHorizontalAlign_Center;
    m_wasPressed      = false;
    m_pressedAtMs     = 0;
}

void UIControl_Button::SetRightIcon(eCONTROLLER_BUTTON button)
{
    m_rightIcon = button;
}

void UIControl_Button::SetRightLabel(const std::wstring &rightLabel, uint32_t color)
{
    m_rightLabel      = rightLabel;
    m_rightLabelColor = color;
}

#endif
