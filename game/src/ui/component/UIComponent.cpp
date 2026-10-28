#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent.h"

UIComponent::UIComponent()
{
    m_visible = true;
}

UIComponent::~UIComponent()
{}

void UIComponent::Tick()
{}

void UIComponent::Render()
{}

void UIComponent::ToggleVisible()
{
    m_visible = !m_visible;
}

void UIComponent::SetVisible(bool visible)
{
    m_visible = visible;
}

bool UIComponent::IsVisible() const
{
    return m_visible;
}

#endif
