#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

class CompoundTag;

enum eGAME_RULE : uint8_t
{
    eGameRule_DoDaylightCycle               = 0,
    eGameRule_WaterSourceConversion         = 1,
    eGameRule_SendCommandFeedback           = 2,
    eGameRule_LogAdminCommands              = 3,
    eGameRule_CommandModificationBlockLimit = 4,
    eGameRule_DoWeatherCycle                = 5,
    eGameRule_RandomTickSpeed               = 6,
    eGameRule_KeepInventory                 = 7,
    eGameRule_DoImmediateRespawn            = 8,
    eGameRule_DoMobSpawning                 = 9,
    eGameRule_PlayersSleepingPercentage     = 10,
    eGameRule_MaxEntityCramming             = 11,
    eGameRule_NaturalRegeneration           = 12,
    eGameRule_MinecartMaxSpeed              = 13,
    eGameRule_Count                         = 14,
};

class GameRules
{
public:
    GameRules();

    static const char *GetName(eGAME_RULE rule);
    static bool IsBoolean(eGAME_RULE rule);
    static bool FindByName(const std::string &name, eGAME_RULE *rule);

    bool GetBool(eGAME_RULE rule) const;
    int GetInt(eGAME_RULE rule) const;
    void Set(eGAME_RULE rule, int value);
    bool Parse(eGAME_RULE rule, const std::string &text, int *value, nlohmann::json *errorMessage) const;
    std::string ToString(eGAME_RULE rule) const;

    void Save(CompoundTag *tag) const;
    void Load(const CompoundTag *tag);

private:
    struct Definition
    {
        const char *name;
        bool boolean;
        int defaultValue;
        int minimum;
    };

    static const Definition s_definitions[eGameRule_Count];

    int m_values[eGameRule_Count];
};
