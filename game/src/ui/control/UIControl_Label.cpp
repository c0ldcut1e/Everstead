#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_Label.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../rendering/Font.h"
#include "../Screen.h"

const float UIControl_Label::s_labelTextScale   = 0.85f;
const float UIControl_Label::s_labelLineSpacing = 12.0f;

UIControl_Label::UIControl_Label(float x, float y, float width, float height, const std::wstring &text, uint32_t color, bool shadow)
    : UIControl(x, y, width, height)
{
    m_text            = text;
    m_color           = color;
    m_shadow          = shadow;
    m_shadowColor     = 0xFF484848;
    m_outline         = false;
    m_textScale       = s_labelTextScale;
    m_horizontalAlign = eHorizontalAlign_Left;
    m_preferredWidth  = {std::max(width, 0.0f)};
    m_preferredHeight = {std::max(height, 0.0f)};
    m_wrapMode        = eWrapMode_None;
    m_wrapWidth       = 0.0f;
    m_lineSpacing     = s_labelLineSpacing;
}

UIControl_Label::~UIControl_Label()
{}

void UIControl_Label::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    Font *font = everstead->GetDefaultFont();
    if (!font || m_text.empty())
    {
        return;
    }

    int actualWidth                 = everstead->GetWidth();
    int actualHeight                = everstead->GetHeight();
    Screen::Rect rect               = GetActualRect(actualWidth, actualHeight);
    float uiScale                   = Screen::ScaleUniform(actualWidth, actualHeight);
    float fontScale                 = font->SnapScale(uiScale * m_textScale);
    float lineHeight                = font->GetLineHeight(fontScale);
    float ascent                    = font->GetAscent(fontScale);
    std::vector<std::wstring> lines = ComputeWrappedLines();
    float lineSpacing               = m_lineSpacing * uiScale;
    float lineAdvance               = lineHeight + lineSpacing;
    float totalTextHeight           = lineHeight * (float) lines.size() + lineSpacing * (float) (lines.size() - 1);
    float firstLineTop              = rect.y + (rect.height - totalTextHeight) * 0.5f;

    font->SetNearest(true);
    for (size_t i = 0; i < lines.size(); i++)
    {
        float textX = round(rect.x);
        if (m_horizontalAlign != eHorizontalAlign_Left)
        {
            float lineWidth = font->GetWidth(lines[i], fontScale);
            if (m_horizontalAlign == eHorizontalAlign_Center)
            {
                textX = round(rect.x + (rect.width - lineWidth) * 0.5f);
            }
            else
            {
                textX = round(rect.x + rect.width - lineWidth);
            }
        }

        float baselineY = round(firstLineTop + lineAdvance * (float) i + ascent);
        if (m_outline)
        {
            float o = std::max(1.0f, round(2.0f * uiScale));
            font->Draw(lines[i], textX - o, baselineY, fontScale, 0xFF000000);
            font->Draw(lines[i], textX + o, baselineY, fontScale, 0xFF000000);
            font->Draw(lines[i], textX, baselineY - o, fontScale, 0xFF000000);
            font->Draw(lines[i], textX, baselineY + o, fontScale, 0xFF000000);
            font->Draw(lines[i], textX - o, baselineY - o, fontScale, 0xFF000000);
            font->Draw(lines[i], textX + o, baselineY - o, fontScale, 0xFF000000);
            font->Draw(lines[i], textX - o, baselineY + o, fontScale, 0xFF000000);
            font->Draw(lines[i], textX + o, baselineY + o, fontScale, 0xFF000000);
        }
        else if (m_shadow)
        {
            font->DrawShadow(lines[i], textX, baselineY, fontScale, m_color, m_shadowColor);
            continue;
        }

        font->Draw(lines[i], textX, baselineY, fontScale, m_color);
    }
}

float UIControl_Label::GetPreferredWidth() const
{
    if (m_preferredWidth > 0.0f)
    {
        return m_preferredWidth;
    }

    return MeasureTextWidth();
}

float UIControl_Label::GetPreferredHeight() const
{
    std::vector<std::wstring> lines = ComputeWrappedLines();
    if (lines.size() > 1)
    {
        Everstead *everstead = Everstead::GetInstance();
        Font *font           = everstead->GetDefaultFont();
        float lineHeight     = font->GetLineHeight(m_textScale);
        return lineHeight * (float) lines.size() + m_lineSpacing * (float) (lines.size() - 1);
    }

    if (m_preferredHeight > 0.0f)
    {
        return m_preferredHeight;
    }

    return 20.0f;
}

void UIControl_Label::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_Label::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

bool UIControl_Label::IsSelectable() const
{
    return false;
}

void UIControl_Label::ResizeHorizontal(float)
{}

void UIControl_Label::ResizeVertical(float)
{}

void UIControl_Label::SetText(const std::wstring &text)
{
    m_text = text;
}

const std::wstring &UIControl_Label::GetText() const
{
    return m_text;
}

void UIControl_Label::SetColor(uint32_t color)
{
    m_color = color;
}

void UIControl_Label::SetShadow(bool shadow)
{
    m_shadow = shadow;
}

void UIControl_Label::SetShadowColor(uint32_t shadowColor)
{
    m_shadowColor = shadowColor;
}

void UIControl_Label::SetOutline(bool outline)
{
    m_outline = outline;
}

bool UIControl_Label::HasOutline() const
{
    return m_outline;
}

void UIControl_Label::SetTextScale(float textScale)
{
    m_textScale = std::max(textScale, 0.01f);
}

float UIControl_Label::GetTextScale() const
{
    return m_textScale;
}

void UIControl_Label::SetHorizontalAlign(eHORIZONTAL_ALIGN align)
{
    m_horizontalAlign = align;
}

UIControl_Label::eHORIZONTAL_ALIGN UIControl_Label::GetHorizontalAlign() const
{
    return m_horizontalAlign;
}

void UIControl_Label::SetWrapMode(eWRAP_MODE wrapMode)
{
    m_wrapMode = wrapMode;
}

UIControl_Label::eWRAP_MODE UIControl_Label::GetWrapMode() const
{
    return m_wrapMode;
}

void UIControl_Label::SetWrapWidth(float wrapWidth)
{
    m_wrapWidth = std::max(wrapWidth, 0.0f);
}

float UIControl_Label::GetWrapWidth() const
{
    return m_wrapWidth;
}

void UIControl_Label::SetLineSpacing(float lineSpacing)
{
    m_lineSpacing = std::max(lineSpacing, 0.0f);
}

float UIControl_Label::GetLineSpacing() const
{
    return m_lineSpacing;
}

float UIControl_Label::MeasureTextWidth() const
{
    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();
    return font->GetWidth(m_text, m_textScale);
}

float UIControl_Label::GetEffectiveWrapWidth() const
{
    if (m_wrapWidth > 0.0f)
    {
        return m_wrapWidth;
    }

    if (m_preferredWidth > 0.0f)
    {
        return m_preferredWidth;
    }

    if (m_width > 0.0f)
    {
        return m_width;
    }

    return 0.0f;
}

std::vector<std::wstring> UIControl_Label::ComputeWrappedLines() const
{
    std::vector<std::wstring> lines;

    if (m_text.empty())
    {
        return lines;
    }

    std::vector<std::wstring> segments;
    {
        std::wstring current;
        for (wchar_t ch : m_text)
        {
            if (ch == L'\n')
            {
                segments.push_back(current);
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }

        segments.push_back(current);
    }

    if (m_wrapMode == eWrapMode_None)
    {
        for (const std::wstring &segment : segments)
        {
            lines.push_back(segment);
        }

        return lines;
    }

    float wrapWidth = GetEffectiveWrapWidth();
    if (wrapWidth <= 0.0f)
    {
        for (const std::wstring &segment : segments)
        {
            lines.push_back(segment);
        }

        return lines;
    }

    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();
    float uiScale        = Screen::ScaleUniform(everstead->GetWidth(), everstead->GetHeight());
    float fontScale      = font->SnapScale(uiScale * m_textScale);
    wrapWidth *= uiScale;

    for (const std::wstring &segment : segments)
    {
        if (segment.empty())
        {
            lines.push_back(L"");
            continue;
        }

        if (m_wrapMode == eWrapMode_Word)
        {
            std::wstring currentLine;
            size_t cursor = 0;
            while (cursor < segment.size())
            {
                size_t nextSpace = segment.find(L' ', cursor);
                size_t wordEnd   = nextSpace == std::wstring::npos ? segment.size() : nextSpace;
                std::wstring word(segment, cursor, wordEnd - cursor);

                std::wstring candidate = currentLine.empty() ? word : currentLine + L' ' + word;
                if (font->GetWidth(candidate, fontScale) <= wrapWidth || currentLine.empty())
                {
                    currentLine = candidate;
                }
                else
                {
                    lines.push_back(currentLine);
                    currentLine = word;
                }

                cursor = wordEnd + 1;
            }

            if (!currentLine.empty())
            {
                lines.push_back(currentLine);
            }
        }
        else
        {
            std::wstring currentLine;
            for (wchar_t ch : segment)
            {
                std::wstring candidate = currentLine + ch;
                if (font->GetWidth(candidate, fontScale) <= wrapWidth || currentLine.empty())
                {
                    currentLine = candidate;
                }
                else
                {
                    lines.push_back(currentLine);
                    currentLine = std::wstring(1, ch);
                }
            }

            if (!currentLine.empty())
            {
                lines.push_back(currentLine);
            }
        }
    }

    if (lines.empty())
    {
        lines.push_back(m_text);
    }

    return lines;
}

#endif
