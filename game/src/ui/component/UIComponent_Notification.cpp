#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Notification.h"

#include <algorithm>

#include "../../core/Everstead.h"
#include "../../utils/Time.h"
#include "../Screen.h"
#include "../control/UIControl_Label.h"

UIComponent_Notification::UIComponent_Notification()
{
    m_multiList    = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f, 0.96f);
    m_openProgress = 0.0f;
    m_holdTimer    = 0.0f;
    m_active       = false;
    m_closing      = false;

    m_multiList->SetPanelVisible(true);
    m_multiList->SetContentPadding(16.0f, 12.0f);
    m_multiList->SetRowSpacing(4.0f);
    m_multiList->SetMaxVisibleElements(2);
}

UIComponent_Notification::~UIComponent_Notification()
{
    delete m_multiList;
    m_multiList = nullptr;
}

void UIComponent_Notification::Show(const std::wstring &title, const std::wstring &message, float duration)
{
    m_active       = true;
    m_closing      = false;
    m_openProgress = 0.0f;
    m_holdTimer    = std::max(duration, 0.0f);

    float labelWidth = 440.0f - 16.0f * 2.0f;
    m_multiList->ClearEntries();
    m_multiList->SetLabelRowWidth(labelWidth);

    if (!title.empty())
    {
        m_multiList->SetLabelRowHeight(30.0f);
        UIControl_Label *titleLabel = m_multiList->AddNewLabel(s_notificationTitleId, title);
        titleLabel->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);
        titleLabel->SetTextScale(0.82f);
    }

    m_multiList->SetLabelRowHeight(44.0f);
    UIControl_Label *messageLabel = m_multiList->AddNewLabel(s_notificationMessageId, message);
    messageLabel->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);
    messageLabel->SetTextScale(0.72f);
    messageLabel->SetWrapMode(UIControl_Label::eWrapMode_Word);
    messageLabel->SetWrapWidth(labelWidth);
}

void UIComponent_Notification::Dismiss()
{
    if (m_active)
    {
        m_closing = true;
    }
}

bool UIComponent_Notification::IsActive() const
{
    return m_active;
}

void UIComponent_Notification::Render()
{
    if (!m_active)
    {
        return;
    }

    int width  = Everstead::GetInstance()->GetWidth();
    int height = Everstead::GetInstance()->GetHeight();
    if (width <= 0 || height <= 0)
    {
        return;
    }

    float delta                = (float) Time::GetDelta();
    bool deactivateAfterRender = false;
    if (m_closing)
    {
        m_openProgress -= delta * 6.0f;
        if (m_openProgress <= 0.0f)
        {
            m_openProgress        = 0.0f;
            deactivateAfterRender = true;
        }
    }
    else
    {
        m_openProgress = std::min(m_openProgress + delta * 6.0f, 1.0f);
        if (m_openProgress >= 1.0f)
        {
            m_holdTimer -= delta;
            if (m_holdTimer <= 0.0f)
            {
                m_closing = true;
            }
        }
    }

    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical(12.0f);

    float uniformBase   = std::min(Screen::ScaleX(width), Screen::ScaleY(height));
    float scaleAdjust   = 1.0f + Screen::SCALE_ADJUST;
    float scale         = Screen::ScaleUniform(width, height);
    float verticalInset = ((float) height - Screen::HEIGHT * uniformBase) * 0.5f;
    float centerY       = (float) height * 0.5f;
    float hiddenActualY = -Screen::ToActualLength(m_multiList->GetHeight(), width, height) - 4.0f * scale;
    float shownActualY  = 18.0f * scale;
    float actualY       = hiddenActualY + (shownActualY - hiddenActualY) * m_openProgress;
    float virtualY      = ((actualY - centerY) / scaleAdjust + centerY - verticalInset) / uniformBase;
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, virtualY);
    m_multiList->Render();

    if (deactivateAfterRender)
    {
        m_active  = false;
        m_closing = false;
    }
}

#endif
