#include "PlayerTeam.h"

#include <memory>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../tag/StringTag.h"

PlayerTeam::PlayerTeam(const std::string &teamName)
{
    name                   = teamName;
    displayName            = nlohmann::json{{"text", teamName}};
    prefix                 = nlohmann::json{{"text", ""}};
    suffix                 = nlohmann::json{{"text", ""}};
    color                  = "reset";
    friendlyFire           = true;
    seeFriendlyInvisibles  = true;
    nameTagVisibility      = eTeamVisibility_Always;
    deathMessageVisibility = eTeamVisibility_Always;
    collisionRule          = eTeamCollisionRule_Always;
}

void PlayerTeam::Save(CompoundTag *tag) const
{
    tag->PutString("Name", name);
    tag->PutString("DisplayName", displayName.dump());
    tag->PutString("MemberNamePrefix", prefix.dump());
    tag->PutString("MemberNameSuffix", suffix.dump());
    tag->PutString("TeamColor", color);
    tag->PutBoolean("AllowFriendlyFire", friendlyFire);
    tag->PutBoolean("SeeFriendlyInvisibles", seeFriendlyInvisibles);
    tag->PutString("NameTagVisibility", kTeamVisibilityNames[nameTagVisibility]);
    tag->PutString("DeathMessageVisibility", kTeamVisibilityNames[deathMessageVisibility]);
    tag->PutString("CollisionRule", kTeamCollisionRuleNames[collisionRule]);

    std::unique_ptr<ListTag> players(new ListTag());
    for (const std::string &member : members)
    {
        players->Add(std::make_unique<StringTag>(member));
    }

    tag->Put("Players", std::move(players));
}

void PlayerTeam::Load(const CompoundTag *tag)
{
    displayName           = nlohmann::json::parse(tag->GetString("DisplayName"), nullptr, false);
    prefix                = nlohmann::json::parse(tag->GetString("MemberNamePrefix"), nullptr, false);
    suffix                = nlohmann::json::parse(tag->GetString("MemberNameSuffix"), nullptr, false);
    color                 = tag->GetString("TeamColor");
    friendlyFire          = tag->GetBoolean("AllowFriendlyFire");
    seeFriendlyInvisibles = tag->GetBoolean("SeeFriendlyInvisibles");

    std::string nameTag   = tag->GetString("NameTagVisibility");
    std::string deathTag  = tag->GetString("DeathMessageVisibility");
    std::string collision = tag->GetString("CollisionRule");
    for (int i = 0; i < (int) std::size(kTeamVisibilityNames); i++)
    {
        if (nameTag == kTeamVisibilityNames[i])
        {
            nameTagVisibility = (eTEAM_VISIBILITY) i;
        }

        if (deathTag == kTeamVisibilityNames[i])
        {
            deathMessageVisibility = (eTEAM_VISIBILITY) i;
        }

        if (collision == kTeamCollisionRuleNames[i])
        {
            collisionRule = (eTEAM_COLLISION_RULE) i;
        }
    }

    ListTag *players = tag->GetList("Players");
    for (size_t i = 0; players && i < players->Size(); i++)
    {
        members.insert(((const StringTag *) players->Get(i))->GetValue());
    }
}

nlohmann::json PlayerTeam::FormatName(const nlohmann::json &memberName) const
{
    nlohmann::json component{{"text", ""}, {"extra", nlohmann::json::array({prefix, memberName, suffix})}};
    if (color != "reset")
    {
        component["color"] = color;
    }

    return component;
}

nlohmann::json PlayerTeam::GetFormattedDisplayName() const
{
    nlohmann::json component{{"text", "["}, {"extra", nlohmann::json::array({displayName, "]"})}};
    if (color != "reset")
    {
        component["color"] = color;
    }

    return component;
}
