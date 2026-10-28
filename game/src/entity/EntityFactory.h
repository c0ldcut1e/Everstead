#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../core/ResourceLocation.h"
#include "EntityType.h"

class Entity;
class Level;
class Random;

class EntityFactory
{
public:
    static void Init();
    static void Register(const ResourceLocation &location, eENTITY_TYPE type, Entity *(*create)(Level *level), int (*spawnData)(Random *random));

    static std::unique_ptr<Entity> Create(eENTITY_TYPE type, Level *level);
    static int CreateSpawnData(eENTITY_TYPE type, Random *random);
    static bool ByName(const ResourceLocation &location, eENTITY_TYPE *type);
    static const ResourceLocation &GetResourceLocation(eENTITY_TYPE type);
    static void GetSpawnableNames(std::vector<std::string> *names);

private:
    struct Entry
    {
        ResourceLocation location;
        eENTITY_TYPE type;
        Entity *(*create)(Level *level);
        int (*spawnData)(Random *random);
    };

    static const Entry *FindEntry(eENTITY_TYPE type);

    static std::vector<Entry> s_entries;
};
