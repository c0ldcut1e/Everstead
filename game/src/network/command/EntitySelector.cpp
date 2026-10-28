#include "EntitySelector.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>

#include "../../core/ResourceLocation.h"
#include "../../entity/EntityFactory.h"
#include "../../entity/LivingEntity.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/StringUtils.h"
#include "../../utils/math/Mth.h"
#include "../PlayerList.h"
#include "CommandContext.h"

#include <glm/gtx/norm.hpp>

EntitySelector::EntitySelector()
{
    m_variable    = '\0';
    m_playersOnly = false;
    m_origin      = glm::dvec3(0.0);
    m_minDistance = -1.0;
    m_maxDistance = -1.0;
    m_hasVolume   = false;
    m_volume      = glm::dvec3(0.0);
    m_limit       = std::numeric_limits<int>::max();
    m_sort        = eSort_Arbitrary;
    m_typeSet     = false;
    m_nameSet     = false;
    m_teamSet     = false;
    m_gameTypeSet = false;
    m_gameType    = eGameType_Survival;
    m_minLevel    = -std::numeric_limits<double>::infinity();
    m_maxLevel    = std::numeric_limits<double>::infinity();
    m_minPitch    = -std::numeric_limits<double>::infinity();
    m_maxPitch    = std::numeric_limits<double>::infinity();
    m_minYaw      = -std::numeric_limits<double>::infinity();
    m_maxYaw      = std::numeric_limits<double>::infinity();
}

bool EntitySelector::Parse(const std::string &targetText, const CommandContext *context, bool playersOnly, nlohmann::json *errorMessage)
{
    m_variable    = targetText.size() >= 2 ? targetText[1] : '\0';
    m_playersOnly = playersOnly || m_variable == 'a' || m_variable == 'p';
    m_origin      = context->GetPosition();
    m_level       = context->GetLevel();
    if (m_variable == 'p')
    {
        m_limit = 1;
        m_sort  = eSort_Nearest;
    }
    else if (m_variable == 'r')
    {
        m_limit = 1;
        m_sort  = eSort_Random;
    }
    else if (m_variable != 'a' && m_variable != 'e' && m_variable != 's')
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.entity.selector.unknown", {StringUtils::ToUtf16(targetText.substr(0, 2))});
        }

        return false;
    }

    std::string options = targetText.substr(2);
    if (!options.empty())
    {
        if (options[0] != '[' || options.back() != ']')
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.unterminated");
            }

            return false;
        }

        if (!ParseOptions(options.substr(1, options.size() - 2), errorMessage))
        {
            return false;
        }
    }

    // A bare @r only picks players, like @a, unless a type widens it
    if (m_variable == 'r' && !m_typeSet)
    {
        m_playersOnly = true;
    }

    return true;
}

bool EntitySelector::Select(PlayerList *playerList, const CommandContext *context, std::vector<Entity *> *entities, nlohmann::json *errorMessage)
{
    entities->clear();
    if (m_variable == 's')
    {
        Entity *entity = context->GetEntity();
        if (Matches(entity) && (!m_playersOnly || entity->GetType() == eEntityType_ServerPlayer))
        {
            entities->push_back(entity);
        }
    }
    else
    {
        const std::vector<std::unique_ptr<ServerPlayer>> &players = playerList->GetPlayers();
        for (size_t i = 0; i < players.size(); i++)
        {
            if (Matches(players[i].get()))
            {
                entities->push_back(players[i].get());
            }
        }

        if (!m_playersOnly)
        {
            const std::vector<ServerLevel *> &levels = context->GetServer()->GetLevels();
            for (size_t i = 0; i < levels.size(); i++)
            {
                if (IsWorldLimited() && levels[i] != m_level)
                {
                    continue;
                }

                const std::vector<std::unique_ptr<Entity>> &levelEntities = levels[i]->GetEntities();
                for (size_t j = 0; j < levelEntities.size(); j++)
                {
                    if (levelEntities[j]->GetType() != eEntityType_ServerPlayer && Matches(levelEntities[j].get()))
                    {
                        entities->push_back(levelEntities[j].get());
                    }
                }
            }
        }
    }

    if (m_sort == eSort_Nearest || m_sort == eSort_Furthest)
    {
        glm::dvec3 origin = m_origin;
        bool nearest      = m_sort == eSort_Nearest;
        std::stable_sort(entities->begin(), entities->end(),
                         [origin, nearest](Entity *a, Entity *b)
                         {
                             double distanceA = glm::distance2(a->GetPosition(), origin);
                             double distanceB = glm::distance2(b->GetPosition(), origin);
                             return nearest ? distanceA < distanceB : distanceA > distanceB;
                         });
    }
    else if (m_sort == eSort_Random)
    {
        for (size_t i = entities->size(); i > 1; i--)
        {
            std::swap((*entities)[i - 1], (*entities)[(size_t) m_level->GetRandom().NextInt(0, (int) i - 1)]);
        }
    }

    if ((int) entities->size() > m_limit)
    {
        entities->resize((size_t) m_limit);
    }

    if (entities->empty())
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate(m_playersOnly ? "argument.entity.notfound.player" : "argument.entity.notfound.entity");
        }

        return false;
    }

    return true;
}

bool EntitySelector::ParseOptions(const std::string &options, nlohmann::json *errorMessage)
{
    size_t index = 0;
    while (index < options.size())
    {
        size_t equals = options.find('=', index);
        if (equals == std::string::npos)
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.valueless",
                                                          {StringUtils::ToUtf16(StringUtils::Trim(options.substr(index)))});
            }

            return false;
        }

        std::string key = StringUtils::Trim(options.substr(index, equals - index));
        std::string value;
        bool quoted = false;
        index       = equals + 1;
        while (index < options.size() && (quoted || options[index] != ','))
        {
            if (options[index] == '"')
            {
                quoted = !quoted;
            }
            else
            {
                value.push_back(options[index]);
            }

            index++;
        }

        index++;
        if (!ParseOption(key, StringUtils::Trim(value), errorMessage))
        {
            return false;
        }
    }

    return true;
}

bool EntitySelector::ParseOption(const std::string &key, const std::string &value, nlohmann::json *errorMessage)
{
    bool negated           = !value.empty() && value[0] == '!';
    std::string plainValue = negated ? StringUtils::Trim(value.substr(1)) : value;
    std::wstring wideKey   = StringUtils::ToUtf16(key);
    std::wstring wideValue = StringUtils::ToUtf16(plainValue);
    if (key == "type")
    {
        if (m_playersOnly || (!negated && m_typeSet))
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.inapplicable", {wideKey});
            }

            return false;
        }

        eENTITY_TYPE type;
        ResourceLocation location;
        if (!ResourceLocation::TryParse(StringUtils::ToLower(plainValue), &location))
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.type.invalid", {wideValue});
            }

            return false;
        }

        if (location.GetPath() == "player")
        {
            type = eEntityType_ServerPlayer;
        }
        else if (!EntityFactory::ByName(location, &type))
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.type.invalid", {wideValue});
            }

            return false;
        }

        std::vector<eENTITY_TYPE> *types = negated ? &m_excludedTypes : &m_types;
        types->push_back(type);
        if (type == eEntityType_ServerPlayer)
        {
            types->push_back(eEntityType_Player);
        }

        m_typeSet |= !negated;

        return true;
    }

    if (key == "name")
    {
        if (negated)
        {
            m_excludedNames.push_back(wideValue);
        }
        else
        {
            m_name    = wideValue;
            m_nameSet = true;
        }

        return true;
    }

    if (key == "team")
    {
        if (negated)
        {
            m_excludedTeams.push_back(plainValue);
        }
        else
        {
            m_team    = plainValue;
            m_teamSet = true;
        }

        return true;
    }

    if (key == "gamemode")
    {
        eGAME_TYPE gameType;
        if (!GameType::Parse(StringUtils::ToLower(plainValue), &gameType))
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.mode.invalid", {wideValue});
            }

            return false;
        }

        if (negated)
        {
            m_excludedGameTypes.push_back(gameType);
        }
        else
        {
            m_gameType    = gameType;
            m_gameTypeSet = true;
        }

        return true;
    }

    if (negated)
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.entity.options.inapplicable", {wideKey});
        }

        return false;
    }

    if (key == "limit")
    {
        double limit;
        if (!ParseNumber(value, &limit, errorMessage))
        {
            return false;
        }

        if (limit < 1.0 || limit != std::floor(limit))
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.limit.toosmall");
            }

            return false;
        }

        m_limit = limit > (double) std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : (int) limit;
        return true;
    }

    if (key == "sort")
    {
        if (value == "nearest")
        {
            m_sort = eSort_Nearest;
        }
        else if (value == "furthest")
        {
            m_sort = eSort_Furthest;
        }
        else if (value == "random")
        {
            m_sort = eSort_Random;
        }
        else if (value == "arbitrary")
        {
            m_sort = eSort_Arbitrary;
        }
        else
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.entity.options.sort.irreversible", {wideValue});
            }

            return false;
        }

        return true;
    }

    if (key == "x" || key == "y" || key == "z")
    {
        return ParseNumber(value, key == "x" ? &m_origin.x : key == "y" ? &m_origin.y : &m_origin.z, errorMessage);
    }

    if (key == "dx" || key == "dy" || key == "dz")
    {
        m_hasVolume = true;
        return ParseNumber(value, key == "dx" ? &m_volume.x : key == "dy" ? &m_volume.y : &m_volume.z, errorMessage);
    }

    if (key == "distance")
    {
        double minimum;
        double maximum;
        if (!ParseRange(value, &minimum, &maximum, errorMessage))
        {
            return false;
        }

        m_minDistance = std::max(minimum, 0.0);
        m_maxDistance = maximum;
        return true;
    }

    if (key == "level")
    {
        return ParseRange(value, &m_minLevel, &m_maxLevel, errorMessage);
    }

    if (key == "x_rotation")
    {
        return ParseRange(value, &m_minPitch, &m_maxPitch, errorMessage);
    }

    if (key == "y_rotation")
    {
        return ParseRange(value, &m_minYaw, &m_maxYaw, errorMessage);
    }

    if (errorMessage)
    {
        *errorMessage = CommandContext::Translate("argument.entity.options.unknown", {wideKey});
    }

    return false;
}

bool EntitySelector::ParseRange(const std::string &text, double *minimum, double *maximum, nlohmann::json *errorMessage) const
{
    *minimum         = -std::numeric_limits<double>::infinity();
    *maximum         = std::numeric_limits<double>::infinity();
    size_t separator = text.find("..");
    if (separator == std::string::npos)
    {
        if (!ParseNumber(text, minimum, errorMessage))
        {
            return false;
        }

        *maximum = *minimum;
        return true;
    }

    std::string low  = text.substr(0, separator);
    std::string high = text.substr(separator + 2);
    if (low.empty() && high.empty())
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.range.empty");
        }

        return false;
    }

    if ((!low.empty() && !ParseNumber(low, minimum, errorMessage)) || (!high.empty() && !ParseNumber(high, maximum, errorMessage)))
    {
        return false;
    }

    if (*minimum > *maximum)
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.range.swapped");
        }

        return false;
    }

    return true;
}

bool EntitySelector::ParseNumber(const std::string &text, double *value, nlohmann::json *errorMessage) const
{
    errno         = 0;
    char *end     = nullptr;
    double parsed = strtod(text.c_str(), &end);
    if (text.empty() || *end != '\0' || errno == ERANGE || !std::isfinite(parsed))
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("parsing.number.invalid", {StringUtils::ToUtf16(text)});
        }

        return false;
    }

    *value = parsed;
    return true;
}

bool EntitySelector::Matches(Entity *entity) const
{
    LivingEntity *living = dynamic_cast<LivingEntity *>(entity);
    if (living && living->GetHealth() <= 0)
    {
        return false;
    }

    eENTITY_TYPE type = entity->GetType();
    if ((m_typeSet && std::find(m_types.begin(), m_types.end(), type) == m_types.end()) ||
        std::find(m_excludedTypes.begin(), m_excludedTypes.end(), type) != m_excludedTypes.end())
    {
        return false;
    }

    if ((m_nameSet && entity->GetName() != m_name) ||
        std::find(m_excludedNames.begin(), m_excludedNames.end(), entity->GetName()) != m_excludedNames.end())
    {
        return false;
    }

    PlayerTeam *team     = entity->GetTeam();
    std::string teamName = team ? team->name : "";
    if ((m_teamSet && teamName != m_team) || std::find(m_excludedTeams.begin(), m_excludedTeams.end(), teamName) != m_excludedTeams.end())
    {
        return false;
    }

    if (IsWorldLimited() && entity->GetLevel() != m_level)
    {
        return false;
    }

    if (m_maxDistance >= 0.0)
    {
        double distance = glm::distance(entity->GetPosition(), m_origin);
        if (distance < m_minDistance || distance > m_maxDistance)
        {
            return false;
        }
    }

    if (m_hasVolume)
    {
        // The volume reaches one block past the far corner and only has to touch the hitbox
        glm::dvec3 corner    = m_origin + m_volume;
        glm::dvec3 boxMin    = glm::min(m_origin, corner);
        glm::dvec3 boxMax    = glm::max(m_origin, corner) + glm::dvec3(1.0);
        const AABB &bounds   = entity->GetBoundingBox();
        glm::dvec3 entityMin = entity->GetPosition() + bounds.GetMin();
        glm::dvec3 entityMax = entity->GetPosition() + bounds.GetMax();
        if (glm::any(glm::lessThan(entityMax, boxMin)) || glm::any(glm::greaterThan(entityMin, boxMax)))
        {
            return false;
        }
    }

    double pitch = Mth::WrapDegrees(-entity->GetPitch());
    double yaw   = Mth::WrapDegrees(entity->GetYaw() - 90.0f);
    if (pitch < m_minPitch || pitch > m_maxPitch || yaw < m_minYaw || yaw > m_maxYaw)
    {
        return false;
    }

    bool levelFiltered = m_minLevel != -std::numeric_limits<double>::infinity() || m_maxLevel != std::numeric_limits<double>::infinity();
    if (type != eEntityType_ServerPlayer)
    {
        return !levelFiltered && !m_gameTypeSet && m_excludedGameTypes.empty();
    }

    ServerPlayer *player = (ServerPlayer *) entity;
    int level            = player->GetExperienceLevel();
    eGAME_TYPE gameType  = player->GetGameType();
    return level >= m_minLevel && level <= m_maxLevel && (!m_gameTypeSet || gameType == m_gameType) &&
           std::find(m_excludedGameTypes.begin(), m_excludedGameTypes.end(), gameType) == m_excludedGameTypes.end();
}

bool EntitySelector::IsWorldLimited() const
{
    return m_maxDistance >= 0.0 || m_hasVolume;
}
