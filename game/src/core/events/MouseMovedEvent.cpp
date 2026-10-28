#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MouseMovedEvent.h"

MouseMovedEvent::MouseMovedEvent(double deltaX, double deltaY)
{
    m_deltaX = deltaX;
    m_deltaY = deltaY;
}

double MouseMovedEvent::GetDeltaX() const
{
    return m_deltaX;
}

double MouseMovedEvent::GetDeltaY() const
{
    return m_deltaY;
}

#endif
