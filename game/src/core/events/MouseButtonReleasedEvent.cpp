#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MouseButtonReleasedEvent.h"

MouseButtonReleasedEvent::MouseButtonReleasedEvent(int button)
{
    m_button = button;
}

int MouseButtonReleasedEvent::GetButton() const
{
    return m_button;
}

#endif
