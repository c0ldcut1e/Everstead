#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Scrollbar.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../Screen.h"
#include "../UIPanelProperties.h"

class UIPanelProperties_ScrollbarBackground : public UIPanelProperties
{
public:
    UIPanelProperties_ScrollbarBackground() : UIPanelProperties(0.0f, 0.0f, 0.0f, 0.0f, 1.0f)
    {}

    const char *GetTexturePath() const override
    {
        return "assets/texture/ui/control/recess9grid.png";
    }
};

class UIPanelProperties_ScrollbarThumb : public UIPanelProperties
{
public:
    UIPanelProperties_ScrollbarThumb() : UIPanelProperties(0.0f, 0.0f, 0.0f, 0.0f, 1.0f)
    {}

    const char *GetTexturePath() const override
    {
        return "assets/texture/ui/control/panel9grid.png";
    }
};

UIComponent_Scrollbar::UIComponent_Scrollbar() : UIComponent()
{
    m_background   = std::make_unique<UIComponent_Panel>(std::make_unique<UIPanelProperties_ScrollbarBackground>());
    m_thumb        = std::make_unique<UIComponent_Panel>(std::make_unique<UIPanelProperties_ScrollbarThumb>());
    m_upArrow      = std::make_unique<UIComponent_ScrollArrow>(UIComponent_ScrollArrow::eDirection_Up);
    m_downArrow    = std::make_unique<UIComponent_ScrollArrow>(UIComponent_ScrollArrow::eDirection_Down);
    m_x            = 0.0f;
    m_y            = 0.0f;
    m_width        = 0.0f;
    m_height       = 0.0f;
    m_cornerScale  = 1.0f;
    m_visibleRows  = 1;
    m_totalRows    = 1;
    m_scrollOffset = 0;
}

UIComponent_Scrollbar::~UIComponent_Scrollbar()
{}

void UIComponent_Scrollbar::SetBounds(float x, float y, float width, float height, float cornerScale)
{
    m_x           = x;
    m_y           = y;
    m_width       = width;
    m_height      = height;
    m_cornerScale = cornerScale;
}

void UIComponent_Scrollbar::SetRange(int visibleRows, int totalRows)
{
    m_visibleRows  = std::max(visibleRows, 1);
    m_totalRows    = std::max(totalRows, m_visibleRows);
    m_scrollOffset = std::clamp(m_scrollOffset, 0, GetMaxScrollOffset());
}

bool UIComponent_Scrollbar::ScrollBy(int delta)
{
    int scrollOffset = std::clamp(m_scrollOffset + delta, 0, GetMaxScrollOffset());
    if (scrollOffset == m_scrollOffset)
    {
        return false;
    }

    m_scrollOffset = scrollOffset;
    if (delta < 0)
    {
        m_upArrow->Flash();
        m_downArrow->StopFlash();
    }
    else
    {
        m_downArrow->Flash();
        m_upArrow->StopFlash();
    }

    return true;
}

int UIComponent_Scrollbar::GetScrollOffset() const
{
    return m_scrollOffset;
}

int UIComponent_Scrollbar::GetMaxScrollOffset() const
{
    return m_totalRows - m_visibleRows;
}

void UIComponent_Scrollbar::Render()
{
    if (!IsVisible())
    {
        return;
    }

    UIPanelProperties *background = m_background->GetProperties();
    background->x                 = m_x;
    background->y                 = m_y;
    background->width             = m_width;
    background->height            = m_height;
    background->cornerScale       = m_cornerScale;
    m_background->Render();

    float overhang           = 5.0f;
    float trackHeight        = m_height + overhang * 2.0f;
    float thumbHeight        = trackHeight * (float) m_visibleRows / (float) m_totalRows;
    int maxScrollOffset      = GetMaxScrollOffset();
    float thumbProgress      = maxScrollOffset > 0 ? (float) m_scrollOffset / (float) maxScrollOffset : 0.0f;
    UIPanelProperties *thumb = m_thumb->GetProperties();
    thumb->x                 = m_x - overhang;
    thumb->y                 = m_y - overhang + (trackHeight - thumbHeight) * thumbProgress;
    thumb->width             = m_width + overhang * 2.0f;
    thumb->height            = thumbHeight;
    thumb->cornerScale       = m_cornerScale;
    m_thumb->Render();

    Everstead *everstead = Everstead::GetInstance();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();
    float arrowWidth     = round(Screen::ToActualLength(m_width, actualWidth, actualHeight));
    float arrowHeight    = round(m_upArrow->GetHeightForWidth(arrowWidth));
    float gap            = round(Screen::ToActualLength(overhang + m_cornerScale * 3.0f, actualWidth, actualHeight));
    float arrowX         = round(Screen::ToActualX(m_x + m_width * 0.5f, actualWidth, actualHeight) - arrowWidth * 0.5f);
    float top            = round(Screen::ToActualY(m_y, actualWidth, actualHeight));
    float bottom         = round(Screen::ToActualY(m_y + m_height, actualWidth, actualHeight));

    if (m_scrollOffset > 0)
    {
        m_upArrow->SetBounds(arrowX, top - gap - arrowHeight, arrowWidth, arrowHeight);
        m_upArrow->Render();
    }

    if (m_scrollOffset < maxScrollOffset)
    {
        m_downArrow->SetBounds(arrowX, bottom + gap, arrowWidth, arrowHeight);
        m_downArrow->Render();
    }
}

#endif
