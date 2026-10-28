#pragma once

#include "Event.h"

class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(double deltaX, double deltaY);

    double GetDeltaX() const;
    double GetDeltaY() const;

private:
    double m_deltaX;
    double m_deltaY;
};
