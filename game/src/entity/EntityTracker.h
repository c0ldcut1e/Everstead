#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "Entity.h"

class PlayerList;
class TrackedEntity;

class EntityTracker
{
public:
    explicit EntityTracker(PlayerList *playerList);
    ~EntityTracker();

    void Clear();
    void Track(Entity *entity);
    void Untrack(Entity *entity);
    void Tick();
    PlayerList *GetPlayerList() const;

private:
    PlayerList *m_playerList;
    std::unordered_map<Entity *, std::unique_ptr<TrackedEntity>> m_entities;
};
