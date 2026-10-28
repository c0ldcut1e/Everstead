#include "LootTable.h"

#include <algorithm>
#include <cmath>

#include "../../core/Logger.h"
#include "../../utils/Random.h"
#include "../crafting/FurnaceRecipes.h"
#include "../item/Item.h"

bool LootContext::FindProperty(const std::string &name, int *value) const
{
    for (size_t i = 0; i < properties.size(); i++)
    {
        if (properties[i].name == name)
        {
            *value = properties[i].value;
            return true;
        }
    }

    return false;
}

void LootNumber::Parse(const nlohmann::json &json)
{
    type   = eType_Constant;
    value  = 0.0f;
    min    = 0;
    max    = 0;
    trials = 0;
    add    = 0.0f;
    scale  = 1.0f;
    if (json.is_object() && json.contains("min"))
    {
        type = eType_Uniform;
        min  = json["min"].get<int>();
        max  = json["max"].get<int>();
    }
    else if (json.is_object() && json.contains("n"))
    {
        type   = eType_Binomial;
        trials = json["n"].get<int>();
        ParseValue(json["p"]);
    }
    else
    {
        ParseValue(json);
    }
}

void LootNumber::ParseValue(const nlohmann::json &json)
{
    if (json.is_number())
    {
        value = json.get<float>();
        return;
    }

    property = json["property"].get<std::string>();
    add      = json.value("add", 0.0f);
    scale    = json.value("scale", 1.0f);
}

float LootNumber::GetValue(const LootContext &context) const
{
    if (property.empty())
    {
        return value;
    }

    int propertyValue = 0;
    context.FindProperty(property, &propertyValue);
    return ((float) propertyValue + add) * scale;
}

int LootNumber::Roll(const LootContext &context) const
{
    if (type == eType_Uniform)
    {
        return context.random->NextInt(min, max);
    }

    if (type == eType_Binomial)
    {
        float probability = GetValue(context);
        int count         = 0;
        for (int i = 0; i < trials; i++)
        {
            if (context.random->NextFloat() < probability)
            {
                count++;
            }
        }

        return count;
    }

    return (int) std::lround(GetValue(context));
}

int LootNumber::GetMax() const
{
    if (type == eType_Uniform)
    {
        return max;
    }

    if (type == eType_Binomial)
    {
        return trials;
    }

    return property.empty() ? (int) std::lround(value) : 1;
}

void LootCondition::Parse(const nlohmann::json &json)
{
    const std::string &name = json["condition"].get_ref<const std::string &>();
    inverted                = json.value("inverted", false);
    chance                  = 0.0f;
    min                     = 0;
    max                     = 0;
    item                    = nullptr;
    if (name == "random_chance")
    {
        type   = eType_RandomChance;
        chance = json["chance"].get<float>();
    }
    else if (name == "property")
    {
        type     = eType_Property;
        property = json["property"].get<std::string>();
        if (json.contains("value"))
        {
            min = json["value"].get<int>();
            max = min;
        }
        else
        {
            min = json.value("min", INT32_MIN);
            max = json.value("max", INT32_MAX);
        }
    }
    else if (name == "killed_by_player")
    {
        type = eType_KilledByPlayer;
    }
    else if (name == "on_fire")
    {
        type = eType_OnFire;
    }
    else if (name == "match_tool")
    {
        type = eType_MatchTool;
        item = Item::ByName(json["item"].get<std::string>());
    }
    else
    {
        Logger::LogError("Unknown loot condition %s", name.c_str());
        type     = eType_RandomChance;
        inverted = false;
    }
}

bool LootCondition::Test(const LootContext &context) const
{
    bool result = false;
    if (type == eType_RandomChance)
    {
        result = context.random->NextFloat() < chance;
    }
    else if (type == eType_Property)
    {
        int propertyValue = 0;
        result            = context.FindProperty(property, &propertyValue) && propertyValue >= min && propertyValue <= max;
    }
    else if (type == eType_KilledByPlayer)
    {
        result = context.killedByPlayer;
    }
    else if (type == eType_OnFire)
    {
        result = context.onFire;
    }
    else
    {
        result = context.tool && !context.tool->IsEmpty() && context.tool->GetItem() == item;
    }

    return result != inverted;
}

bool LootCondition::TestAll(const std::vector<LootCondition> &conditions, const LootContext &context)
{
    for (size_t i = 0; i < conditions.size(); i++)
    {
        if (!conditions[i].Test(context))
        {
            return false;
        }
    }

    return true;
}

void LootCondition::ParseAll(const nlohmann::json &json, std::vector<LootCondition> *conditions)
{
    if (!json.contains("conditions"))
    {
        return;
    }

    const nlohmann::json &list = json["conditions"];
    conditions->resize(list.size());
    for (size_t i = 0; i < list.size(); i++)
    {
        (*conditions)[i].Parse(list[i]);
    }
}

void LootFunction::Parse(const nlohmann::json &json)
{
    const std::string &name = json["function"].get_ref<const std::string &>();
    if (name != "furnace_smelt")
    {
        Logger::LogError("Unknown loot function %s", name.c_str());
    }

    type = eType_FurnaceSmelt;
    LootCondition::ParseAll(json, &conditions);
}

void LootFunction::Apply(const LootContext &context, ItemInstance *item) const
{
    if (!LootCondition::TestAll(conditions, context))
    {
        return;
    }

    ItemInstance result = FurnaceRecipes::GetInstance()->GetResult(ItemInstance(item->GetItem(), 1, item->GetAuxValue()));
    if (!result.IsEmpty())
    {
        *item = ItemInstance(result.GetItem(), item->GetCount(), result.GetAuxValue());
    }
}

void LootEntry::Parse(const nlohmann::json &json)
{
    item     = nullptr;
    auxValue = 0;
    weight   = json.value("weight", 1);
    count.Parse(json.value("count", nlohmann::json(1)));
    LootCondition::ParseAll(json, &conditions);
    if (json.value("type", std::string("item")) == "empty")
    {
        return;
    }

    const std::string &name = json["name"].get_ref<const std::string &>();
    item                    = Item::ByName(name);
    if (!item)
    {
        Logger::LogError("Unknown loot item %s", name.c_str());
        return;
    }

    auxValue = json.value("data", 0);
    if (json.contains("functions"))
    {
        const nlohmann::json &list = json["functions"];
        functions.resize(list.size());
        for (size_t i = 0; i < list.size(); i++)
        {
            functions[i].Parse(list[i]);
        }
    }
}

void LootPool::Parse(const nlohmann::json &json)
{
    rolls.Parse(json.value("rolls", nlohmann::json(1)));
    LootCondition::ParseAll(json, &conditions);
    const nlohmann::json &list = json["entries"];
    entries.resize(list.size());
    for (size_t i = 0; i < list.size(); i++)
    {
        entries[i].Parse(list[i]);
    }
}

void LootPool::Roll(const LootContext &context, std::vector<ItemInstance> *out) const
{
    if (!LootCondition::TestAll(conditions, context))
    {
        return;
    }

    std::vector<const LootEntry *> candidates;
    int rollCount = rolls.Roll(context);
    for (int roll = 0; roll < rollCount; roll++)
    {
        candidates.clear();
        int totalWeight = 0;
        for (size_t i = 0; i < entries.size(); i++)
        {
            if (LootCondition::TestAll(entries[i].conditions, context))
            {
                candidates.push_back(&entries[i]);
                totalWeight += entries[i].weight;
            }
        }

        if (totalWeight <= 0)
        {
            return;
        }

        int pick               = context.random->NextInt(0, totalWeight - 1);
        const LootEntry *entry = candidates[0];
        for (size_t i = 0; i < candidates.size(); i++)
        {
            pick -= candidates[i]->weight;
            if (pick < 0)
            {
                entry = candidates[i];
                break;
            }
        }

        int count = entry->count.Roll(context);
        if (!entry->item || count <= 0)
        {
            continue;
        }

        ItemInstance item(entry->item, count, entry->auxValue);
        for (size_t i = 0; i < entry->functions.size(); i++)
        {
            entry->functions[i].Apply(context, &item);
        }

        out->push_back(item);
    }
}

LootTable::LootTable()
{
    m_experience.Parse(nlohmann::json(0));
}

void LootTable::Parse(const nlohmann::json &json)
{
    if (json.contains("experience"))
    {
        m_experience.Parse(json["experience"]);
    }

    if (!json.contains("pools"))
    {
        return;
    }

    const nlohmann::json &list = json["pools"];
    m_pools.resize(list.size());
    for (size_t i = 0; i < list.size(); i++)
    {
        m_pools[i].Parse(list[i]);
    }
}

void LootTable::GetRandomItems(const LootContext &context, std::vector<ItemInstance> *out) const
{
    for (size_t i = 0; i < m_pools.size(); i++)
    {
        m_pools[i].Roll(context, out);
    }
}

int LootTable::GetExperience(const LootContext &context) const
{
    return m_experience.Roll(context);
}

void LootTable::GetPossibleItems(std::vector<ItemInstance> *out) const
{
    for (size_t i = 0; i < m_pools.size(); i++)
    {
        const std::vector<LootEntry> &entries = m_pools[i].entries;
        for (size_t j = 0; j < entries.size(); j++)
        {
            if (!entries[j].item)
            {
                continue;
            }

            ItemInstance item(entries[j].item, std::max(entries[j].count.GetMax(), 1), entries[j].auxValue);
            ItemInstance smelted = item;
            for (size_t k = 0; k < entries[j].functions.size(); k++)
            {
                ItemInstance result = FurnaceRecipes::GetInstance()->GetResult(ItemInstance(item.GetItem(), 1, item.GetAuxValue()));
                if (!result.IsEmpty())
                {
                    smelted = ItemInstance(result.GetItem(), item.GetCount(), result.GetAuxValue());
                }
            }

            ItemInstance variants[] = {item, smelted};
            for (int k = 0; k < 2; k++)
            {
                bool found = false;
                for (size_t l = 0; l < out->size(); l++)
                {
                    found = found || ((*out)[l].GetItem() == variants[k].GetItem() && (*out)[l].GetAuxValue() == variants[k].GetAuxValue());
                }

                if (!found)
                {
                    out->push_back(variants[k]);
                }
            }
        }
    }
}
