#pragma once

#include "Event.h"

class MouseButtonReleasedEvent : public Event
{
public:
    explicit MouseButtonReleasedEvent(int button);

    int GetButton() const;

private:
    int m_button;
};
