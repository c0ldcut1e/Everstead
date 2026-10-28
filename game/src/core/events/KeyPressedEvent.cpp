#ifndef EVERSTEAD_DEDICATED_SERVER

#include "KeyPressedEvent.h"

KeyPressedEvent::KeyPressedEvent(int key)
{
    m_key = key;
}

int KeyPressedEvent::GetKey() const
{
    return m_key;
}

#endif
