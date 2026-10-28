#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl.h"

#include "../Screen.h"

UIControl::UIControl(float x, float y, float width, float height) : UIComponent()
{
    m_id              = 0;
    m_x               = x;
    m_y               = y;
    m_width           = width;
    m_height          = height;
    m_opacity         = 1.0f;
    m_enabled         = true;
    m_selected        = false;
    m_hasLayoutOrigin = false;
    m_layoutOriginX   = 0.0f;
    m_layoutOriginY   = 0.0f;
}

UIControl::~UIControl()
{}

void UIControl::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

void UIControl::SetSize(float width, float height)
{
    m_width  = width;
    m_height = height;
}

void UIControl::SetBounds(float x, float y, float width, float height)
{
    m_x      = x;
    m_y      = y;
    m_width  = width;
    m_height = height;
}

void UIControl::SetId(uint32_t id)
{
    m_id = id;
}

void UIControl::SetLayoutOrigin(float x, float y)
{
    m_hasLayoutOrigin = true;
    m_layoutOriginX   = x;
    m_layoutOriginY   = y;
}

void UIControl::ClearLayoutOrigin()
{
    m_hasLayoutOrigin = false;
}

void UIControl::SetOpacity(float opacity)
{
    if (opacity < 0.0f)
    {
        opacity = 0.0f;
    }

    if (opacity > 1.0f)
    {
        opacity = 1.0f;
    }

    m_opacity = opacity;
}

void UIControl::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

float UIControl::GetOpacity() const
{
    return m_opacity;
}

bool UIControl::IsEnabled() const
{
    return m_enabled;
}

uint32_t UIControl::GetId() const
{
    return m_id;
}

float UIControl::GetX() const
{
    return m_x;
}

float UIControl::GetY() const
{
    return m_y;
}

float UIControl::GetWidth() const
{
    return m_width;
}

float UIControl::GetHeight() const
{
    return m_height;
}

Screen::Rect UIControl::GetActualRect(int actualWidth, int actualHeight) const
{
    float x = Screen::ToActualX(m_x, actualWidth, actualHeight);
    float y = Screen::ToActualY(m_y, actualWidth, actualHeight);
    if (m_hasLayoutOrigin)
    {
        x = Screen::ToActualOffsetX(m_layoutOriginX, m_x, actualWidth, actualHeight);
        y = Screen::ToActualOffsetY(m_layoutOriginY, m_y, actualWidth, actualHeight);
    }

    return {x, y, Screen::ToActualLength(m_width, actualWidth, actualHeight), Screen::ToActualLength(m_height, actualWidth, actualHeight)};
}

float UIControl::GetPreferredWidth() const
{
    return m_width;
}

float UIControl::GetPreferredHeight() const
{
    return m_height;
}

void UIControl::ResizeHorizontal(float width)
{
    m_width = width;
}

void UIControl::ResizeVertical(float height)
{
    m_height = height;
}

bool UIControl::IsSelectable() const
{
    return m_enabled;
}

void UIControl::SetSelected(bool selected)
{
    m_selected = selected;
}

bool UIControl::IsSelected() const
{
    return m_selected;
}

void UIControl::Activate()
{}

bool UIControl::StepValue(int)
{
    return false;
}

bool UIControl::OnPointerPressed(int, int, int, int)
{
    return false;
}

bool UIControl::OnPointerMoved(int, int, int, int)
{
    return false;
}

bool UIControl::OnPointerReleased(int, int, int, int)
{
    return false;
}

bool UIControl::ContainsActualPoint(int actualX, int actualY, int actualWidth, int actualHeight) const
{
    if (!IsVisible())
    {
        return false;
    }

    Screen::Rect rect = GetActualRect(actualWidth, actualHeight);
    return (float) actualX >= rect.x && (float) actualX <= rect.x + rect.width && (float) actualY >= rect.y &&
           (float) actualY <= rect.y + rect.height;
}

uint32_t UIControl::ScaleColorAlpha(uint32_t color, float factor)
{
    uint32_t alpha = (uint32_t) ((float) ((color >> 24) & 0xFF) * factor);
    return (alpha << 24) | (color & 0x00FFFFFF);
}

#endif
