#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "LootTable.h"

class Block;
class ResourceLocation;

class LootTables
{
public:
    static void Init();
    static const LootTable *ForBlock(const Block *block);
    static const LootTable *ForEntity(const ResourceLocation &location);
    static const std::map<std::string, LootTable> &GetEntityTables();
    static const LootTable *ForChest(const std::string &name);

private:
    static std::unordered_map<const Block *, LootTable> s_blockTables;
    static std::map<std::string, LootTable> s_entityTables;
    static std::map<std::string, LootTable> s_chestTables;
};
