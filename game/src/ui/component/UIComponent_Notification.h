#pragma once

#include <cstdint>
#include <string>

#include "../control/UIControl_MultiList.h"
#include "UIComponent.h"

class UIComponent_Notification : public UIComponent
{
public:
    UIComponent_Notification();
    ~UIComponent_Notification() override;

    void Show(const std::wstring &title, const std::wstring &message, float duration);
    void Dismiss();
    bool IsActive() const;

    void Render() override;

private:
    static const uint32_t s_notificationTitleId   = 1;
    static const uint32_t s_notificationMessageId = 2;

    UIControl_MultiList *m_multiList;
    float m_openProgress;
    float m_holdTimer;
    bool m_active;
    bool m_closing;
};
