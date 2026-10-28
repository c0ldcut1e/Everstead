#pragma once

#include "Event.h"

class KeyReleasedEvent : public Event
{
public:
    explicit KeyReleasedEvent(int key);

    int GetKey() const;

private:
    int m_key;
};
