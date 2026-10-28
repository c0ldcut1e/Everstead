#ifndef EVERSTEAD_DEDICATED_SERVER

#include "EventManager.h"

std::vector<Event *> EventManager::s_events;
std::vector<EventManager::Listener> EventManager::s_listeners;

void EventManager::Push(Event *event)
{
    s_events.push_back(event);
}

void EventManager::AddListener(const Listener &listener)
{
    s_listeners.push_back(listener);
}

void EventManager::Process()
{
    for (Event *event : s_events)
    {
        for (EventManager::Listener &listener : s_listeners)
        {
            listener(*event);
        }

        delete event;
    }

    s_events.clear();
}

#endif
