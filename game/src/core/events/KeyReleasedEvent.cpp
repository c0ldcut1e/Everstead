#ifndef EVERSTEAD_DEDICATED_SERVER

#include "KeyReleasedEvent.h"

KeyReleasedEvent::KeyReleasedEvent(int key)
{
    m_key = key;
}

int KeyReleasedEvent::GetKey() const
{
    return m_key;
}

#endif
