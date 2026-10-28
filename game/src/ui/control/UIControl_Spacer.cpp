#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_Spacer.h"

#include <algorithm>

UIControl_Spacer::UIControl_Spacer(float x, float y, float width, float height) : UIControl(x, y, width, height)
{
    m_preferredWidth  = {std::max(width, 0.0f)};
    m_preferredHeight = {std::max(height, 0.0f)};
}

UIControl_Spacer::~UIControl_Spacer()
{}

void UIControl_Spacer::Render()
{}

float UIControl_Spacer::GetPreferredWidth() const
{
    return std::max(m_preferredWidth, 0.0f);
}

float UIControl_Spacer::GetPreferredHeight() const
{
    return std::max(m_preferredHeight, 0.0f);
}

void UIControl_Spacer::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_Spacer::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

bool UIControl_Spacer::IsSelectable() const
{
    return false;
}

#endif
