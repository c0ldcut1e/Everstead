#include "LootTables.h"

#include "../../core/Logger.h"
#include "../../core/ResourceLocation.h"
#include "../../io/File.h"
#include "../block/Block.h"

std::unordered_map<const Block *, LootTable> LootTables::s_blockTables;
std::map<std::string, LootTable> LootTables::s_entityTables;
std::map<std::string, LootTable> LootTables::s_chestTables;

void LootTables::Init()
{
    s_blockTables.clear();
    s_entityTables.clear();
    s_chestTables.clear();
    std::vector<File> blockFiles = File("assets/loot_table/blocks").ListFiles();
    for (size_t i = 0; i < blockFiles.size(); i++)
    {
        std::string name = blockFiles[i].GetName();
        name             = name.substr(0, name.size() - blockFiles[i].GetExtension().size());
        Block *block     = Block::ByName(name);
        if (!block)
        {
            Logger::LogError("Loot table for unknown block %s", name.c_str());
            continue;
        }

        s_blockTables[block].Parse(nlohmann::json::parse(blockFiles[i].ReadAllText()));
    }

    std::vector<File> entityFiles = File("assets/loot_table/entities").ListFiles();
    for (size_t i = 0; i < entityFiles.size(); i++)
    {
        std::string name = entityFiles[i].GetName();
        s_entityTables[name.substr(0, name.size() - entityFiles[i].GetExtension().size())].Parse(nlohmann::json::parse(entityFiles[i].ReadAllText()));
    }

    std::vector<File> chestFiles = File("assets/loot_table/chests").ListFiles();
    for (size_t i = 0; i < chestFiles.size(); i++)
    {
        std::string name = chestFiles[i].GetName();
        s_chestTables[name.substr(0, name.size() - chestFiles[i].GetExtension().size())].Parse(nlohmann::json::parse(chestFiles[i].ReadAllText()));
    }
}

const LootTable *LootTables::ForBlock(const Block *block)
{
    auto it = s_blockTables.find(block);
    return it == s_blockTables.end() ? nullptr : &it->second;
}

const LootTable *LootTables::ForEntity(const ResourceLocation &location)
{
    auto it = s_entityTables.find(location.GetPath());
    return it == s_entityTables.end() ? nullptr : &it->second;
}

const std::map<std::string, LootTable> &LootTables::GetEntityTables()
{
    return s_entityTables;
}

const LootTable *LootTables::ForChest(const std::string &name)
{
    return &s_chestTables.at(name);
}
