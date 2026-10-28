#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_Slider.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "../../core/Everstead.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/math/Mth.h"
#include "../Screen.h"

const float UIControl_Slider::s_sliderDefaultWidth       = 550.0f;
const float UIControl_Slider::s_sliderDefaultHeight      = 40.0f;
const float UIControl_Slider::s_sliderTextScale          = 0.8f;
const float UIControl_Slider::s_sliderSourceBorderPixels = 2.0f;

void UIControl_Slider::EmitSliderQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                                      uint32_t color)
{
    if (x1 <= x0 || y1 <= y0)
    {
        return;
    }

    builder->Color(color);
    builder->VertexUV(x0, y0, 0.0f, u0, v0);
    builder->VertexUV(x1, y0, 0.0f, u1, v0);
    builder->VertexUV(x1, y1, 0.0f, u1, v1);
    builder->VertexUV(x0, y0, 0.0f, u0, v0);
    builder->VertexUV(x1, y1, 0.0f, u1, v1);
    builder->VertexUV(x0, y1, 0.0f, u0, v1);
}

void UIControl_Slider::EmitSliderTextureQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
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

void UIControl_Slider::EmitSliderNineSlice(BufferBuilder *builder, float x, float y, float width, float height, float textureWidth,
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

    EmitSliderTextureQuad(builder, x0, y0, x1, y1, u0, v0, u1, v1, color, region);
    EmitSliderTextureQuad(builder, x1, y0, x2, y1, u1, v0, u2, v1, color, region);
    EmitSliderTextureQuad(builder, x2, y0, x3, y1, u2, v0, u3, v1, color, region);
    EmitSliderTextureQuad(builder, x0, y1, x1, y2, u0, v1, u1, v2, color, region);
    EmitSliderTextureQuad(builder, x1, y1, x2, y2, u1, v1, u2, v2, color, region);
    EmitSliderTextureQuad(builder, x2, y1, x3, y2, u2, v1, u3, v2, color, region);
    EmitSliderTextureQuad(builder, x0, y2, x1, y3, u0, v2, u1, v3, color, region);
    EmitSliderTextureQuad(builder, x1, y2, x2, y3, u1, v2, u2, v3, color, region);
    EmitSliderTextureQuad(builder, x2, y2, x3, y3, u2, v2, u3, v3, color, region);
}

UIControl_Slider::UIControl_Slider(float x, float y, float width, float height, const std::wstring &name, float minValue, float maxValue,
                                   float defaultValue, float increment)
    : UIControl_Slider(x, y, width, height, name, minValue, maxValue, defaultValue, increment, "assets/texture/ui/control/slider_background.png",
                       "assets/texture/ui/control/slider_background_highlight.png", "assets/texture/ui/control/slider_graphic.png")
{}

UIControl_Slider::~UIControl_Slider()
{}

void UIControl_Slider::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    Font *font = everstead->GetDefaultFont();

    int actualWidth             = everstead->GetWidth();
    int actualHeight            = everstead->GetHeight();
    Screen::Rect rect           = GetActualRect(actualWidth, actualHeight);
    float uiScale               = Screen::ScaleUniform(actualWidth, actualHeight);
    float roundedX              = round(rect.x);
    float roundedY              = round(rect.y);
    float roundedWidth          = round(rect.width);
    float roundedHeight         = round(rect.height);
    float backgroundPixelHeight = std::max(1.0f, (float) std::max(m_backgroundRegion.pixelHeight, 1));
    float backgroundPixelScale  = std::max(1.0f, round(roundedHeight / backgroundPixelHeight));
    float backgroundBorderSize  = backgroundPixelScale * s_sliderSourceBorderPixels;
    float outlineThickness      = 3.0f;
    float graphicHeight         = roundedHeight;
    float graphicWidth = round(graphicHeight * ((float) std::max(m_graphicRegion.pixelWidth, 1) / (float) std::max(m_graphicRegion.pixelHeight, 1)));
    float trackWidth   = std::max(0.0f, roundedWidth - graphicWidth);
    float graphicX     = round(roundedX + trackWidth * GetNormalizedValue());
    float graphicY     = roundedY;

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    GlStateManager::SetAlphaCutoff(0.01f);
    EmitSliderNineSlice(builder, roundedX, roundedY, roundedWidth, roundedHeight, (float) std::max(m_backgroundRegion.pixelWidth, 1),
                        backgroundPixelHeight, s_sliderSourceBorderPixels, s_sliderSourceBorderPixels, backgroundBorderSize, backgroundBorderSize,
                        0xFFFFFFFF, m_backgroundRegion);
    builder->End();
    Textures::Unbind();

    if (m_selected)
    {
        float highlightX      = roundedX + backgroundBorderSize;
        float highlightY      = roundedY + backgroundBorderSize;
        float highlightWidth  = std::max(0.0f, roundedWidth - backgroundBorderSize * 2.0f);
        float highlightHeight = std::max(0.0f, roundedHeight - backgroundBorderSize * 2.0f);

        builder->Begin(ePrimitiveType_Triangles);
        Textures::Bind(UITextureAtlas::GetTexture()->GetId());
        GlStateManager::SetAlphaCutoff(0.01f);
        EmitSliderTextureQuad(builder, highlightX, highlightY, highlightX + highlightWidth, highlightY + highlightHeight, 0.0f, 0.0f, 1.0f, 1.0f,
                              0xFFFFFFFF, m_highlightRegion);
        builder->End();
        Textures::Unbind();

        builder->Begin(ePrimitiveType_Triangles);
        Textures::Unbind();
        GlStateManager::SetAlphaCutoff(-1.0f);
        float outlineX0 = roundedX - outlineThickness;
        float outlineY0 = roundedY - outlineThickness;
        float outlineX1 = roundedX + roundedWidth + outlineThickness;
        float outlineY1 = roundedY + roundedHeight + outlineThickness;

        EmitSliderQuad(builder, outlineX0, outlineY0, outlineX1, roundedY, 0.0f, 0.0f, 0.0f, 0.0f, s_sliderSelectedOutlineColor);
        EmitSliderQuad(builder, outlineX0, roundedY + roundedHeight, outlineX1, outlineY1, 0.0f, 0.0f, 0.0f, 0.0f, s_sliderSelectedOutlineColor);
        EmitSliderQuad(builder, outlineX0, roundedY, roundedX, roundedY + roundedHeight, 0.0f, 0.0f, 0.0f, 0.0f, s_sliderSelectedOutlineColor);
        EmitSliderQuad(builder, roundedX + roundedWidth, roundedY, outlineX1, roundedY + roundedHeight, 0.0f, 0.0f, 0.0f, 0.0f,
                       s_sliderSelectedOutlineColor);
        builder->End();
    }

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    GlStateManager::SetAlphaCutoff(0.01f);
    EmitSliderTextureQuad(builder, graphicX, graphicY, graphicX + graphicWidth, graphicY + graphicHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF,
                          m_graphicRegion);
    builder->End();
    Textures::Unbind();
    GlStateManager::SetAlphaCutoff(-1.0f);

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();

    if (!m_text.empty())
    {
        float fontScale     = font->SnapScale(uiScale * s_sliderTextScale);
        float textWidth     = font->GetWidth(m_text, fontScale);
        float textX         = round(rect.x + (rect.width - textWidth) * 0.5f);
        float textBaselineY = round(rect.y + (rect.height - font->GetLineHeight(fontScale)) * 0.5f + font->GetAscent(fontScale));
        uint32_t textColor  = m_selected ? s_sliderSelectedTextColor : s_sliderDefaultTextColor;

        font->SetNearest(true);
        font->Draw(m_text, textX + 2.0f, textBaselineY + 2.0f, fontScale, 0xFF000000);
        font->Draw(m_text, textX, textBaselineY, fontScale, textColor);
    }
}

float UIControl_Slider::GetPreferredWidth() const
{
    if (m_preferredWidth > 0.0f)
    {
        return m_preferredWidth;
    }

    return s_sliderDefaultWidth;
}

float UIControl_Slider::GetPreferredHeight() const
{
    if (m_preferredHeight > 0.0f)
    {
        return m_preferredHeight;
    }

    return s_sliderDefaultHeight;
}

void UIControl_Slider::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_Slider::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

bool UIControl_Slider::StepValue(int direction)
{
    if (direction == 0)
    {
        return false;
    }

    return SetValue(m_value + m_increment * (float) direction);
}

bool UIControl_Slider::OnPointerPressed(int actualX, int, int actualWidth, int actualHeight)
{
    m_dragging = true;
    return UpdateValueFromActualX(actualX, actualWidth, actualHeight);
}

bool UIControl_Slider::OnPointerMoved(int actualX, int, int actualWidth, int actualHeight)
{
    if (!m_dragging)
    {
        return false;
    }

    return UpdateValueFromActualX(actualX, actualWidth, actualHeight);
}

bool UIControl_Slider::OnPointerReleased(int actualX, int, int actualWidth, int actualHeight)
{
    bool changed = false;
    if (m_dragging)
    {
        changed = UpdateValueFromActualX(actualX, actualWidth, actualHeight);
    }

    m_dragging = false;
    return changed;
}

bool UIControl_Slider::SetValue(float value)
{
    float clampedValue = QuantizeValue(value);
    if (fabs(m_value - clampedValue) < 0.0001f)
    {
        return false;
    }

    m_value = clampedValue;
    return true;
}

float UIControl_Slider::GetValue() const
{
    return m_value;
}

float UIControl_Slider::GetMinimumValue() const
{
    return m_minValue;
}

float UIControl_Slider::GetMaximumValue() const
{
    return m_maxValue;
}

float UIControl_Slider::GetDefaultValue() const
{
    return m_defaultValue;
}

float UIControl_Slider::GetIncrement() const
{
    return m_increment;
}

const std::wstring &UIControl_Slider::GetName() const
{
    return m_name;
}

void UIControl_Slider::ResetToDefaultValue()
{
    SetValue(m_defaultValue);
}

void UIControl_Slider::SetText(const std::wstring &text)
{
    m_text = text;
}

const std::wstring &UIControl_Slider::GetText() const
{
    return m_text;
}

float UIControl_Slider::GetNormalizedValue() const
{
    float range = m_maxValue - m_minValue;
    if (range <= 0.0001f)
    {
        return 0.0f;
    }

    return std::clamp((m_value - m_minValue) / range, 0.0f, 1.0f);
}

float UIControl_Slider::QuantizeValue(float value) const
{
    float clampedValue = std::clamp(value, m_minValue, m_maxValue);
    if (m_increment <= 0.0001f)
    {
        return clampedValue;
    }

    float steps = round((clampedValue - m_minValue) / m_increment);
    return std::clamp(m_minValue + steps * m_increment, m_minValue, m_maxValue);
}

bool UIControl_Slider::UpdateValueFromActualX(int actualX, int actualWidth, int actualHeight)
{
    Screen::Rect rect     = GetActualRect(actualWidth, actualHeight);
    float graphicWidth    = rect.height * ((float) m_graphicRegion.pixelWidth / (float) m_graphicRegion.pixelHeight);
    float travelWidth     = std::max(1.0f, rect.width - graphicWidth);
    float centeredX       = (float) actualX - rect.x - graphicWidth * 0.5f;
    float normalizedValue = std::clamp(centeredX / travelWidth, 0.0f, 1.0f);
    return SetValue(m_minValue + normalizedValue * (m_maxValue - m_minValue));
}

UIControl_Slider::UIControl_Slider(float x, float y, float width, float height, const std::wstring &name, float minValue, float maxValue,
                                   float defaultValue, float increment, const char *backgroundTexturePath, const char *highlightTexturePath,
                                   const char *graphicTexturePath)
    : UIControl(x, y, width, height)
{
    m_name             = name;
    m_text             = name;
    m_value            = 0.0f;
    m_minValue         = std::min(minValue, maxValue);
    m_maxValue         = std::max(minValue, maxValue);
    m_defaultValue     = defaultValue;
    m_increment        = std::max(increment, 0.0f);
    m_preferredWidth   = std::max(width, 0.0f);
    m_preferredHeight  = std::max(height, 0.0f);
    m_dragging         = false;
    m_backgroundRegion = UITextureAtlas::GetRegion(backgroundTexturePath);
    m_highlightRegion  = UITextureAtlas::GetRegion(highlightTexturePath);
    m_graphicRegion    = UITextureAtlas::GetRegion(graphicTexturePath);

    if (fabsf(m_maxValue - m_minValue) < 0.0001f)
    {
        m_maxValue = m_minValue + 1.0f;
    }

    m_defaultValue = QuantizeValue(defaultValue);
    m_value        = m_defaultValue;
}

#endif
