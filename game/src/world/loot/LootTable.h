#pragma once

#include <string>
#include <vector>

#include "../item/ItemInstance.h"

#include <nlohmann/json.hpp>

class Item;
class Random;

struct LootProperty
{
    std::string name;
    int value;
};

struct LootContext
{
    Random *random;
    std::vector<LootProperty> properties;
    const ItemInstance *tool;
    bool killedByPlayer;
    bool onFire;

    bool FindProperty(const std::string &name, int *value) const;
};

struct LootNumber
{
    enum eTYPE : uint8_t
    {
        eType_Constant,
        eType_Uniform,
        eType_Binomial
    };

    eTYPE type;
    float value;
    int min;
    int max;
    int trials;
    std::string property;
    float add;
    float scale;

    void Parse(const nlohmann::json &json);
    int Roll(const LootContext &context) const;
    int GetMax() const;
    void ParseValue(const nlohmann::json &json);
    float GetValue(const LootContext &context) const;
};

struct LootCondition
{
    enum eTYPE : uint8_t
    {
        eType_RandomChance,
        eType_Property,
        eType_KilledByPlayer,
        eType_OnFire,
        eType_MatchTool
    };

    eTYPE type;
    bool inverted;
    float chance;
    std::string property;
    int min;
    int max;
    Item *item;

    void Parse(const nlohmann::json &json);
    bool Test(const LootContext &context) const;
    static bool TestAll(const std::vector<LootCondition> &conditions, const LootContext &context);
    static void ParseAll(const nlohmann::json &json, std::vector<LootCondition> *conditions);
};

struct LootFunction
{
    enum eTYPE : uint8_t
    {
        eType_FurnaceSmelt
    };

    eTYPE type;
    std::vector<LootCondition> conditions;

    void Parse(const nlohmann::json &json);
    void Apply(const LootContext &context, ItemInstance *item) const;
};

struct LootEntry
{
    Item *item;
    int auxValue;
    int weight;
    LootNumber count;
    std::vector<LootCondition> conditions;
    std::vector<LootFunction> functions;

    void Parse(const nlohmann::json &json);
};

struct LootPool
{
    LootNumber rolls;
    std::vector<LootCondition> conditions;
    std::vector<LootEntry> entries;

    void Parse(const nlohmann::json &json);
    void Roll(const LootContext &context, std::vector<ItemInstance> *out) const;
};

class LootTable
{
public:
    LootTable();

    void Parse(const nlohmann::json &json);
    void GetRandomItems(const LootContext &context, std::vector<ItemInstance> *out) const;
    int GetExperience(const LootContext &context) const;
    void GetPossibleItems(std::vector<ItemInstance> *out) const;

private:
    std::vector<LootPool> m_pools;
    LootNumber m_experience;
};
