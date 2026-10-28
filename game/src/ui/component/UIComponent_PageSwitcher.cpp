#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_PageSwitcher.h"

#include <string>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../rendering/Font.h"
#include "../Screen.h"
#include "../control/UIController.h"

UIComponent_PageSwitcher::UIComponent_PageSwitcher() : UIComponent()
{
    m_leftButton  = std::make_unique<UIControl_Button>(0.0f, 0.0f, 0.0f, 0.0f, L"<");
    m_rightButton = std::make_unique<UIControl_Button>(0.0f, 0.0f, 0.0f, 0.0f, L">");
    m_x           = 0.0f;
    m_y           = 0.0f;
    m_width       = 0.0f;
    m_buttonSize  = 0.0f;
    m_cornerScale = 1.0f;
    m_page        = 0;
    m_pageCount   = 1;
    m_rendered    = false;
}

UIComponent_PageSwitcher::~UIComponent_PageSwitcher()
{}

void UIComponent_PageSwitcher::SetBounds(float x, float y, float width, float buttonSize, float cornerScale)
{
    m_x           = x;
    m_y           = y;
    m_width       = width;
    m_buttonSize  = buttonSize;
    m_cornerScale = cornerScale;
}

void UIComponent_PageSwitcher::SetPage(int page, int pageCount)
{
    m_page      = page;
    m_pageCount = pageCount;
}

int UIComponent_PageSwitcher::GetDirectionAt(float virtualX, float virtualY) const
{
    if (!IsVisible() || m_pageCount <= 1 || virtualY < m_y || virtualY >= m_y + m_buttonSize)
    {
        return 0;
    }

    if (m_page > 0 && virtualX >= m_x && virtualX < m_x + m_buttonSize)
    {
        return -1;
    }

    if (m_page < m_pageCount - 1 && virtualX >= m_x + m_width - m_buttonSize && virtualX < m_x + m_width)
    {
        return 1;
    }

    return 0;
}

void UIComponent_PageSwitcher::Render()
{
    if (!IsVisible() || m_pageCount <= 1)
    {
        return;
    }

    if (m_page > 0)
    {
        RenderButton(m_leftButton.get(), m_x);
    }

    if (m_page < m_pageCount - 1)
    {
        RenderButton(m_rightButton.get(), m_x + m_width - m_buttonSize);
    }

    Everstead *everstead = Everstead::GetInstance();
    int actualWidth      = everstead->GetWidth();
    int actualHeight     = everstead->GetHeight();
    Font *font           = everstead->GetDefaultFont();
    float fontScale      = font->SnapScale(Screen::ScaleUniform(actualWidth, actualHeight) * m_cornerScale);
    std::wstring text    = std::to_wstring(m_page + 1) + L"/" + std::to_wstring(m_pageCount);
    float textX          = Screen::ToActualX(m_x + m_width * 0.5f, actualWidth, actualHeight) - font->GetWidth(text, fontScale) * 0.5f;
    float textY          = Screen::ToActualY(m_y + m_buttonSize * 0.5f, actualWidth, actualHeight) - font->GetLineHeight(fontScale) * 0.5f;
    font->SetNearest(true);
    font->DrawShadow(text, textX, textY + font->GetAscent(fontScale), fontScale, 0xFFFFFFFF);
    m_rendered = true;
}

void UIComponent_PageSwitcher::RenderButton(UIControl_Button *button, float x)
{
    Everstead *everstead = Everstead::GetInstance();
    int mouseX           = 0;
    int mouseY           = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    button->SetBounds(x, m_y, m_buttonSize, m_buttonSize);
    button->SetPreferredWidth(m_buttonSize);
    bool hovered = button->ContainsActualPoint(mouseX, mouseY, everstead->GetWidth(), everstead->GetHeight());
    if (hovered && !button->IsSelected() && m_rendered)
    {
        everstead->GetUiController()->PlayUiSound("ui.focus");
    }

    button->SetSelected(hovered);
    button->Render();
}

#endif
