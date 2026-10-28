#pragma once

#include "Event.h"

class MouseButtonPressedEvent : public Event
{
public:
    explicit MouseButtonPressedEvent(int button);

    int GetButton() const;

private:
    int m_button;
};
