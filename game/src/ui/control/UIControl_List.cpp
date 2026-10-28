#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_List.h"

#include <algorithm>
#include <memory>

#include "../UIPanelProperties.h"

class UIPanelProperties_List : public UIPanelProperties
{
public:
    UIPanelProperties_List(float x, float y, float width, float height, float transparency) : UIPanelProperties(x, y, width, height, transparency)
    {}

    ~UIPanelProperties_List() override
    {}

    const char *GetTexturePath() const override
    {
        return "assets/texture/ui/control/panel9grid.png";
    }
};

UIControl_List::UIControl_List(float x, float y, float width, float height, float transparency) : UIControl(x, y, width, height)
{
    m_panel                = {std::make_unique<UIComponent_Panel>(std::make_unique<UIPanelProperties_List>(x, y, width, height, transparency))};
    m_transparency         = transparency;
    m_contentPaddingX      = 24.0f;
    m_contentPaddingY      = 24.0f;
    m_scene                = nullptr;
    m_currentSelection     = -1;
    m_hasPanelBoundsOffset = false;
    m_panelOffsetX         = 0.0f;
    m_panelOffsetY         = 0.0f;
    m_panelOffsetWidth     = 0.0f;
    m_panelOffsetHeight    = 0.0f;
}

UIControl_List::~UIControl_List()
{
    ClearElements();
}

void UIControl_List::Tick()
{
    SyncPanel();

    for (UIControl *control : m_controls)
    {
        if (ShouldProcessControl(control))
        {
            control->SetLayoutOrigin(m_x, m_y);
            control->Tick();
        }
    }
}

void UIControl_List::Render()
{
    SyncPanel();

    if (m_panel)
    {
        m_panel->Render();
    }

    for (UIControl *control : m_controls)
    {
        if (ShouldProcessControl(control))
        {
            control->SetLayoutOrigin(m_x, m_y);
            control->Render();
        }
    }
}

void UIControl_List::Init()
{}

void UIControl_List::SetupControl(UIScene *scene)
{
    m_scene = scene;
    Init();
}

void UIControl_List::AddControl(UIControl *control)
{
    if (!control)
    {
        return;
    }

    m_controls.push_back(control);
    control->SetLayoutOrigin(m_x, m_y);
}

void UIControl_List::RemoveControl(UIControl *control)
{
    if (!control)
    {
        return;
    }

    control->ClearLayoutOrigin();
    m_controls.erase(std::remove(m_controls.begin(), m_controls.end(), control), m_controls.end());
}

UIControl *UIControl_List::AddElement(eUI_CONTROL_TYPE, uint32_t)
{
    return nullptr;
}

void UIControl_List::ClearElements()
{
    for (UIControl *control : m_controls)
    {
        if (!control)
        {
            continue;
        }

        control->ClearLayoutOrigin();
        delete control;
    }

    m_controls.clear();
    m_currentSelection = -1;
}

void UIControl_List::ClearList()
{
    ClearElements();
}

UIControl *UIControl_List::GetControlById(uint32_t id) const
{
    for (UIControl *control : m_controls)
    {
        if (control && control->GetId() == id)
        {
            return control;
        }
    }

    return nullptr;
}

UIControl *UIControl_List::GetTopItem() const
{
    if (m_controls.empty())
    {
        return nullptr;
    }

    return m_controls.front();
}

int UIControl_List::GetItemCount() const
{
    return (int) m_controls.size();
}

int UIControl_List::GetListIndex(const UIControl *control) const
{
    if (!control)
    {
        return -1;
    }

    for (size_t i = 0; i < m_controls.size(); i++)
    {
        if (m_controls[i] == control)
        {
            return (int) i;
        }
    }

    return -1;
}

void UIControl_List::SetCurrentSelection(int index)
{
    m_currentSelection = index;
}

int UIControl_List::GetCurrentSelection() const
{
    return m_currentSelection;
}

UIScene *UIControl_List::GetScene() const
{
    return m_scene;
}

void UIControl_List::SetPanelVisible(bool visible)
{
    if (!m_panel)
    {
        return;
    }

    m_panel->SetVisible(visible);
}

bool UIControl_List::IsPanelVisible() const
{
    if (!m_panel)
    {
        return false;
    }

    return m_panel->IsVisible();
}

Screen::Rect UIControl_List::GetPanelActualRect(int actualWidth, int actualHeight) const
{
    if (!m_hasPanelBoundsOffset)
    {
        return GetActualRect(actualWidth, actualHeight);
    }

    float panelX = Screen::ToActualOffsetX(m_x, m_panelOffsetX, actualWidth, actualHeight);
    float panelY = Screen::ToActualOffsetY(m_y, m_panelOffsetY, actualWidth, actualHeight);
    return {panelX, panelY, Screen::ToActualLength(m_panelOffsetWidth, actualWidth, actualHeight),
            Screen::ToActualLength(m_panelOffsetHeight, actualWidth, actualHeight)};
}

void UIControl_List::SetPanelBoundsOffset(float x, float y, float width, float height)
{
    m_hasPanelBoundsOffset = true;
    m_panelOffsetX         = x;
    m_panelOffsetY         = y;
    m_panelOffsetWidth     = width;
    m_panelOffsetHeight    = height;
}

void UIControl_List::ClearPanelBoundsOffset()
{
    m_hasPanelBoundsOffset = false;
    m_panelOffsetX         = 0.0f;
    m_panelOffsetY         = 0.0f;
    m_panelOffsetWidth     = 0.0f;
    m_panelOffsetHeight    = 0.0f;
}

void UIControl_List::ResizeVertical(float padding)
{
    float maxBottom = m_y + m_contentPaddingY;
    for (UIControl *control : m_controls)
    {
        if (!ShouldProcessControl(control))
        {
            continue;
        }

        maxBottom = std::max(maxBottom, control->GetY() + control->GetPreferredHeight());
    }

    m_height = std::max(m_height, maxBottom - m_y + padding);
}

void UIControl_List::ResizeHorizontal(float padding)
{
    float maxRight = m_x + m_contentPaddingX;
    for (UIControl *control : m_controls)
    {
        if (!ShouldProcessControl(control))
        {
            continue;
        }

        maxRight = std::max(maxRight, control->GetX() + control->GetPreferredWidth());
    }

    m_width = std::max(m_width, maxRight - m_x + padding);
}

void UIControl_List::SyncPanel()
{
    if (!m_panel)
    {
        return;
    }

    UIPanelProperties *properties = m_panel->GetProperties();
    if (!properties)
    {
        return;
    }

    properties->x            = m_hasPanelBoundsOffset ? m_x + m_panelOffsetX : m_x;
    properties->y            = m_hasPanelBoundsOffset ? m_y + m_panelOffsetY : m_y;
    properties->width        = m_hasPanelBoundsOffset ? m_panelOffsetWidth : m_width;
    properties->height       = m_hasPanelBoundsOffset ? m_panelOffsetHeight : m_height;
    properties->transparency = m_transparency;
}

bool UIControl_List::ShouldProcessControl(const UIControl *control) const
{
    return control && control->IsVisible();
}

#endif
