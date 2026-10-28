#pragma once

#include <cstdint>
#include <memory>

#include "UIComponent.h"
#include "UIComponent_Panel.h"
#include "UIComponent_ScrollArrow.h"

class UIComponent_Scrollbar : public UIComponent
{
public:
    UIComponent_Scrollbar();
    ~UIComponent_Scrollbar() override;

    void Render() override;

    void SetBounds(float x, float y, float width, float height, float cornerScale);
    void SetRange(int visibleRows, int totalRows);
    bool ScrollBy(int delta);
    int GetScrollOffset() const;
    int GetMaxScrollOffset() const;

private:
    std::unique_ptr<UIComponent_Panel> m_background;
    std::unique_ptr<UIComponent_Panel> m_thumb;
    std::unique_ptr<UIComponent_ScrollArrow> m_upArrow;
    std::unique_ptr<UIComponent_ScrollArrow> m_downArrow;
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    float m_cornerScale;
    int m_visibleRows;
    int m_totalRows;
    int m_scrollOffset;
};
