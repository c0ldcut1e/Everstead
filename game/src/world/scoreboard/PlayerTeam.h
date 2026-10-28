#pragma once

#include <set>
#include <string>

#include <nlohmann/json.hpp>

class CompoundTag;

enum eTEAM_VISIBILITY
{
    eTeamVisibility_Always,
    eTeamVisibility_Never,
    eTeamVisibility_HideForOtherTeams,
    eTeamVisibility_HideForOwnTeam,
};

enum eTEAM_COLLISION_RULE
{
    eTeamCollisionRule_Always,
    eTeamCollisionRule_Never,
    eTeamCollisionRule_PushOtherTeams,
    eTeamCollisionRule_PushOwnTeam,
};

constexpr const char *kTeamVisibilityNames[]    = {"always", "never", "hideForOtherTeams", "hideForOwnTeam"};
constexpr const char *kTeamCollisionRuleNames[] = {"always", "never", "pushOtherTeams", "pushOwnTeam"};

struct PlayerTeam
{
    PlayerTeam(const std::string &teamName);

    void Save(CompoundTag *tag) const;
    void Load(const CompoundTag *tag);
    nlohmann::json FormatName(const nlohmann::json &memberName) const;
    nlohmann::json GetFormattedDisplayName() const;

    std::string name;
    nlohmann::json displayName;
    nlohmann::json prefix;
    nlohmann::json suffix;
    std::string color;
    bool friendlyFire;
    bool seeFriendlyInvisibles;
    eTEAM_VISIBILITY nameTagVisibility;
    eTEAM_VISIBILITY deathMessageVisibility;
    eTEAM_COLLISION_RULE collisionRule;
    std::set<std::string> members;
};
