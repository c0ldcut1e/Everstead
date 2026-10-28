#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MouseWheelEvent.h"

MouseWheelEvent::MouseWheelEvent(int wheelY)
{
    m_wheelY = wheelY;
}

int MouseWheelEvent::GetWheelY() const
{
    return m_wheelY;
}

#endif
