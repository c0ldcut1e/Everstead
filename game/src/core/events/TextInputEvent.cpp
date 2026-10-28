#ifndef EVERSTEAD_DEDICATED_SERVER

#include "TextInputEvent.h"

TextInputEvent::TextInputEvent(const char *text)
{
    m_text = text ? text : "";
}

const std::string &TextInputEvent::GetText() const
{
    return m_text;
}

#endif
