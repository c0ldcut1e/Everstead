#pragma once

#include "Event.h"

class WindowResizedEvent : public Event
{
public:
    WindowResizedEvent(int width, int height);

    int GetWidth() const;
    int GetHeight() const;

private:
    int m_width;
    int m_height;
};
