#pragma once

#include <string>

#include "Event.h"

class TextInputEvent : public Event
{
public:
    explicit TextInputEvent(const char *text);

    const std::string &GetText() const;

private:
    std::string m_text;
};
