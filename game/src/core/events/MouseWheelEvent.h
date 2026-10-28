#pragma once

#include "Event.h"

class MouseWheelEvent : public Event
{
public:
    explicit MouseWheelEvent(int wheelY);

    int GetWheelY() const;

private:
    int m_wheelY;
};
