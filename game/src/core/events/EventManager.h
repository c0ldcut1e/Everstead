#pragma once

#include <functional>
#include <vector>

#include "Event.h"

class EventManager
{
public:
    using Listener = std::function<void(Event &)>;

    static void Push(Event *event);
    static void AddListener(const Listener &listener);
    static void Process();

private:
    static std::vector<Event *> s_events;
    static std::vector<Listener> s_listeners;
};
