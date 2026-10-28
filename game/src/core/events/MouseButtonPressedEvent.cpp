#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MouseButtonPressedEvent.h"

MouseButtonPressedEvent::MouseButtonPressedEvent(int button)
{
    m_button = button;
}

int MouseButtonPressedEvent::GetButton() const
{
    return m_button;
}

#endif
