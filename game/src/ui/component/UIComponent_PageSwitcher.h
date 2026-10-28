#pragma once

#include <memory>

#include "../control/UIControl_Button.h"
#include "UIComponent.h"

class UIComponent_PageSwitcher : public UIComponent
{
public:
    UIComponent_PageSwitcher();
    ~UIComponent_PageSwitcher() override;

    void Render() override;

    void SetBounds(float x, float y, float width, float buttonSize, float cornerScale);
    void SetPage(int page, int pageCount);
    int GetDirectionAt(float virtualX, float virtualY) const;

private:
    void RenderButton(UIControl_Button *button, float x);

    std::unique_ptr<UIControl_Button> m_leftButton;
    std::unique_ptr<UIControl_Button> m_rightButton;
    float m_x;
    float m_y;
    float m_width;
    float m_buttonSize;
    float m_cornerScale;
    int m_page;
    int m_pageCount;
    bool m_rendered;
};
