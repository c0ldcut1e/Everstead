#pragma once

#include <string>
#include <vector>

#include "../../entity/EntityType.h"
#include "../../entity/player/GameType.h"
#include "../../utils/Random.h"
#include "../PlayerList.h"

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

class CommandContext;

class EntitySelector
{
public:
    EntitySelector();

    bool Parse(const std::string &targetText, const CommandContext *context, bool playersOnly, nlohmann::json *errorMessage);
    bool Select(PlayerList *playerList, const CommandContext *context, std::vector<Entity *> *entities, nlohmann::json *errorMessage);

private:
    enum eSORT
    {
        eSort_Arbitrary,
        eSort_Nearest,
        eSort_Furthest,
        eSort_Random,
    };

    bool ParseOptions(const std::string &options, nlohmann::json *errorMessage);
    bool ParseOption(const std::string &key, const std::string &value, nlohmann::json *errorMessage);
    bool ParseRange(const std::string &text, double *minimum, double *maximum, nlohmann::json *errorMessage) const;
    bool ParseNumber(const std::string &text, double *value, nlohmann::json *errorMessage) const;
    bool Matches(Entity *entity) const;
    bool IsWorldLimited() const;

    char m_variable;
    bool m_playersOnly;
    glm::dvec3 m_origin;
    Level *m_level;
    double m_minDistance;
    double m_maxDistance;
    bool m_hasVolume;
    glm::dvec3 m_volume;
    int m_limit;
    eSORT m_sort;
    bool m_typeSet;
    std::vector<eENTITY_TYPE> m_types;
    std::vector<eENTITY_TYPE> m_excludedTypes;
    bool m_nameSet;
    std::wstring m_name;
    std::vector<std::wstring> m_excludedNames;
    bool m_teamSet;
    std::string m_team;
    std::vector<std::string> m_excludedTeams;
    bool m_gameTypeSet;
    eGAME_TYPE m_gameType;
    std::vector<eGAME_TYPE> m_excludedGameTypes;
    double m_minLevel;
    double m_maxLevel;
    double m_minPitch;
    double m_maxPitch;
    double m_minYaw;
    double m_maxYaw;
};
