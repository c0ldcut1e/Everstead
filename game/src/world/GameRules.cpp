#include "GameRules.h"

#include <cerrno>
#include <climits>
#include <cstdlib>

#include "../tag/CompoundTag.h"

const GameRules::Definition GameRules::s_definitions[eGameRule_Count] = {
        {"doDaylightCycle", true, 1, 0},
        {"waterSourceConversion", true, 1, 0},
        {"sendCommandFeedback", true, 1, 0},
        {"logAdminCommands", true, 1, 0},
        {"commandModificationBlockLimit", false, 32768, 1},
        {"doWeatherCycle", true, 1, 0},
        {"randomTickSpeed", false, 3, 0},
        {"keepInventory", true, 0, 0},
        {"doImmediateRespawn", true, 0, 0},
        {"doMobSpawning", true, 1, 0},
        {"playersSleepingPercentage", false, 100, 0},
        {"maxEntityCramming", false, 24, 0},
        {"naturalRegeneration", true, 1, 0},
        {"minecartMaxSpeed", false, 8, 1},
};

GameRules::GameRules()
{
    for (int i = 0; i < eGameRule_Count; i++)
    {
        m_values[i] = s_definitions[i].defaultValue;
    }
}

const char *GameRules::GetName(eGAME_RULE rule)
{
    return s_definitions[rule].name;
}

bool GameRules::IsBoolean(eGAME_RULE rule)
{
    return s_definitions[rule].boolean;
}

bool GameRules::FindByName(const std::string &name, eGAME_RULE *rule)
{
    for (int i = 0; i < eGameRule_Count; i++)
    {
        if (name == s_definitions[i].name)
        {
            *rule = (eGAME_RULE) i;
            return true;
        }
    }

    return false;
}

bool GameRules::GetBool(eGAME_RULE rule) const
{
    return m_values[rule] != 0;
}

int GameRules::GetInt(eGAME_RULE rule) const
{
    return m_values[rule];
}

void GameRules::Set(eGAME_RULE rule, int value)
{
    m_values[rule] = value;
}

bool GameRules::Parse(eGAME_RULE rule, const std::string &text, int *value, nlohmann::json *errorMessage) const
{
    if (s_definitions[rule].boolean)
    {
        if (text != "true" && text != "false")
        {
            *errorMessage = {{"translate", "parsing.bool.invalid"}, {"with", nlohmann::json::array({text})}};
            return false;
        }

        *value = text == "true" ? 1 : 0;
        return true;
    }

    errno       = 0;
    char *end   = nullptr;
    long parsed = strtol(text.c_str(), &end, 10);
    if (text.empty() || *end != '\0' || errno == ERANGE || parsed > INT_MAX)
    {
        *errorMessage = {{"translate", "parsing.int.invalid"}, {"with", nlohmann::json::array({text})}};
        return false;
    }

    if (parsed < s_definitions[rule].minimum)
    {
        *errorMessage = {{"translate", "argument.integer.low"},
                         {"with", nlohmann::json::array({std::to_string(s_definitions[rule].minimum), std::to_string(parsed)})}};
        return false;
    }

    *value = (int) parsed;
    return true;
}

std::string GameRules::ToString(eGAME_RULE rule) const
{
    if (s_definitions[rule].boolean)
    {
        return m_values[rule] != 0 ? "true" : "false";
    }

    return std::to_string(m_values[rule]);
}

void GameRules::Save(CompoundTag *tag) const
{
    for (int i = 0; i < eGameRule_Count; i++)
    {
        tag->PutString(s_definitions[i].name, ToString((eGAME_RULE) i));
    }
}

void GameRules::Load(const CompoundTag *tag)
{
    for (int i = 0; i < eGameRule_Count; i++)
    {
        int value = 0;
        nlohmann::json errorMessage;
        if (tag->Contains(s_definitions[i].name) && Parse((eGAME_RULE) i, tag->GetString(s_definitions[i].name), &value, &errorMessage))
        {
            m_values[i] = value;
        }
    }
}
