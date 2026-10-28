#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIGroup.h"

#include <algorithm>

UIGroup::UIGroup(const std::string &name)
{
    m_name          = name;
    m_components    = {};
    m_tickEnabled   = true;
    m_inputEnabled  = true;
    m_renderEnabled = true;
}

UIGroup::~UIGroup()
{
    for (UIComponent *component : m_components)
    {
        if (component)
        {
            delete component;
        }
    }

    m_components.clear();
}

void UIGroup::Tick()
{
    if (!m_tickEnabled)
    {
        return;
    }

    for (UIComponent *component : m_components)
    {
        if (component && component->IsVisible())
        {
            component->Tick();
        }
    }
}

void UIGroup::TickInput()
{
    if (!m_inputEnabled)
    {
        return;
    }
}

void UIGroup::Render()
{
    if (!m_renderEnabled)
    {
        return;
    }

    for (UIComponent *component : m_components)
    {
        if (component && component->IsVisible())
        {
            component->Render();
        }
    }
}

void UIGroup::AddComponent(UIComponent *component)
{
    if (!component)
    {
        return;
    }

    m_components.push_back(component);
}

void UIGroup::RemoveComponent(UIComponent *component)
{
    if (!component)
    {
        return;
    }

    m_components.erase(std::remove(m_components.begin(), m_components.end(), component), m_components.end());
}

void UIGroup::SetTickEnabled(bool enabled)
{
    m_tickEnabled = enabled;
}

bool UIGroup::IsTickEnabled() const
{
    return m_tickEnabled;
}

void UIGroup::SetInputEnabled(bool enabled)
{
    m_inputEnabled = enabled;
}

bool UIGroup::IsInputEnabled() const
{
    return m_inputEnabled;
}

void UIGroup::SetRenderEnabled(bool enabled)
{
    m_renderEnabled = enabled;
}

bool UIGroup::IsRenderEnabled() const
{
    return m_renderEnabled;
}

const std::string &UIGroup::GetName() const
{
    return m_name;
}

#endif
