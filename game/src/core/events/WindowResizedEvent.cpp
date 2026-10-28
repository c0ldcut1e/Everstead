#ifndef EVERSTEAD_DEDICATED_SERVER

#include "WindowResizedEvent.h"

WindowResizedEvent::WindowResizedEvent(int width, int height)
{
    m_width  = width;
    m_height = height;
}

int WindowResizedEvent::GetWidth() const
{
    return m_width;
}

int WindowResizedEvent::GetHeight() const
{
    return m_height;
}

#endif
