#include "EntityTracker.h"

#include "TrackedEntity.h"

#include "../network/PlayerList.h"

EntityTracker::EntityTracker(PlayerList *playerList)
{
    m_playerList = playerList;
}

EntityTracker::~EntityTracker()
{
    Clear();
}

void EntityTracker::Clear()
{
    m_entities.clear();
}

void EntityTracker::Track(Entity *entity)
{
    std::unique_ptr<TrackedEntity> &tracked = m_entities[entity];
    if (!tracked)
    {
        tracked.reset(new TrackedEntity(entity));
    }
}

void EntityTracker::Untrack(Entity *entity)
{
    m_entities.erase(entity);
}

void EntityTracker::Tick()
{
    for (std::pair<Entity *const, std::unique_ptr<TrackedEntity>> &tracked : m_entities)
    {
        tracked.second->Tick(this);
    }
}

PlayerList *EntityTracker::GetPlayerList() const
{
    return m_playerList;
}
