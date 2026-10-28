#include "TeamCommand.h"

#include <algorithm>
#include <cctype>

#include "../../tag/CompoundTag.h"
#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "../packet/s2c/TeamsS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

static const char *const kModifyOptions[] = {"collisionRule",     "color",  "deathMessageVisibility", "displayName", "friendlyFire",
                                             "nametagVisibility", "prefix", "seeFriendlyInvisibles",  "suffix"};
static const char *const kColorNames[] = {"aqua", "black", "blue",  "dark_aqua",    "dark_blue", "dark_gray", "dark_green", "dark_purple", "dark_red",
                                          "gold", "gray",  "green", "light_purple", "red",       "reset",     "white",      "yellow"};

static nlohmann::json TranslateWith(const std::string &key, const std::vector<nlohmann::json> &arguments)
{
    nlohmann::json component = CommandContext::Translate(key);
    for (size_t i = 0; i < arguments.size(); i++)
    {
        component["with"].push_back(arguments[i]);
    }

    return component;
}

static nlohmann::json JoinComponents(const std::vector<nlohmann::json> &components)
{
    nlohmann::json list = {{"text", ""}, {"extra", nlohmann::json::array()}};
    for (size_t i = 0; i < components.size(); i++)
    {
        if (i > 0)
        {
            list["extra"].push_back(", ");
        }

        list["extra"].push_back(components[i]);
    }

    return list;
}

TeamCommand::TeamCommand() : Command("team")
{}

TeamCommand::~TeamCommand()
{}

bool TeamCommand::RequiresOperator() const
{
    return true;
}

void TeamCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    Scoreboard &scoreboard = context->GetLevel()->GetScoreboard();
    std::string action     = arguments.Count() >= 2 ? StringUtils::ToLower(arguments.Get(1)) : "";
    size_t count           = arguments.Count();

    if (action == "list" && count <= 3)
    {
        if (count == 3)
        {
            PlayerTeam *team = FindTeam(context, arguments.Get(2));
            if (!team)
            {
                return;
            }

            if (team->members.empty())
            {
                context->SendSystemMessage(TranslateWith("commands.team.list.members.empty", {team->GetFormattedDisplayName()}), false);
                return;
            }

            std::vector<nlohmann::json> names(team->members.begin(), team->members.end());
            context->SendSystemMessage(TranslateWith("commands.team.list.members.success",
                                                     {team->GetFormattedDisplayName(), std::to_string(names.size()), JoinComponents(names)}),
                                       false);
            return;
        }

        const std::map<std::string, PlayerTeam> &teams = scoreboard.GetTeams();
        if (teams.empty())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.team.list.teams.empty"), false);
            return;
        }

        std::vector<nlohmann::json> names;
        for (const std::pair<const std::string, PlayerTeam> &entry : teams)
        {
            names.push_back(entry.second.GetFormattedDisplayName());
        }

        context->SendSystemMessage(TranslateWith("commands.team.list.teams.success", {std::to_string(names.size()), JoinComponents(names)}), false);
        return;
    }

    if (action == "add" && count >= 3)
    {
        const std::string &name = arguments.Get(2);
        for (size_t i = 0; i < name.size(); i++)
        {
            char c = name[i];
            if (!isalnum((unsigned char) c) && c != '_' && c != '-' && c != '.' && c != '+')
            {
                context->SendSystemMessage(CommandContext::Translate("commands.team.usage"), true);
                return;
            }
        }

        if (scoreboard.GetTeam(name))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.team.add.duplicate"), true);
            return;
        }

        nlohmann::json displayName = name;
        if (count > 3 && !ParseComponent(context, arguments.GetRest(3), &displayName))
        {
            return;
        }

        PlayerTeam *team  = scoreboard.AddTeam(name);
        team->displayName = displayName;
        SendTeams(context);
        context->SendSystemMessage(TranslateWith("commands.team.add.success", {team->GetFormattedDisplayName()}), false);
        return;
    }

    if ((action == "remove" || action == "empty") && count == 3)
    {
        PlayerTeam *team = FindTeam(context, arguments.Get(2));
        if (!team)
        {
            return;
        }

        nlohmann::json teamName = team->GetFormattedDisplayName();
        if (action == "remove")
        {
            scoreboard.RemoveTeam(team);
            SendTeams(context);
            context->SendSystemMessage(TranslateWith("commands.team.remove.success", {teamName}), false);
            return;
        }

        if (team->members.empty())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.team.empty.unchanged"), true);
            return;
        }

        std::vector<std::string> members(team->members.begin(), team->members.end());
        for (size_t i = 0; i < members.size(); i++)
        {
            scoreboard.RemoveMember(members[i]);
        }

        SendTeams(context);
        context->SendSystemMessage(TranslateWith("commands.team.empty.success", {std::to_string(members.size()), teamName}), false);
        return;
    }

    if (action == "join" && count >= 3)
    {
        PlayerTeam *team = FindTeam(context, arguments.Get(2));
        if (!team)
        {
            return;
        }

        std::vector<std::string> members;
        if (count == 3)
        {
            Entity *entity = context->GetEntity();
            if (!entity)
            {
                context->SendSystemMessage(CommandContext::Translate("permissions.requires.entity"), true);
                return;
            }

            members.push_back(entity->GetScoreboardName());
        }
        else if (!ResolveMembers(context, arguments, 3, &members))
        {
            return;
        }

        for (size_t i = 0; i < members.size(); i++)
        {
            scoreboard.AddMember(members[i], team);
        }

        SendTeams(context);
        if (members.size() == 1)
        {
            context->SendSystemMessage(TranslateWith("commands.team.join.success.single", {members[0], team->GetFormattedDisplayName()}), false);
        }
        else
        {
            context->SendSystemMessage(
                    TranslateWith("commands.team.join.success.multiple", {std::to_string(members.size()), team->GetFormattedDisplayName()}), false);
        }

        return;
    }

    if (action == "leave" && count >= 3)
    {
        std::vector<std::string> members;
        if (!ResolveMembers(context, arguments, 2, &members))
        {
            return;
        }

        for (size_t i = 0; i < members.size(); i++)
        {
            scoreboard.RemoveMember(members[i]);
        }

        SendTeams(context);
        if (members.size() == 1)
        {
            context->SendSystemMessage(TranslateWith("commands.team.leave.success.single", {members[0]}), false);
        }
        else
        {
            context->SendSystemMessage(CommandContext::Translate("commands.team.leave.success.multiple", {std::to_wstring(members.size())}), false);
        }

        return;
    }

    if (action == "modify" && count >= 5)
    {
        PlayerTeam *team = FindTeam(context, arguments.Get(2));
        if (team)
        {
            Modify(context, team, arguments);
        }

        return;
    }

    context->SendSystemMessage(CommandContext::Translate("commands.team.usage"), true);
}

void TeamCommand::Modify(CommandContext *context, PlayerTeam *team, const CommandArguments &arguments)
{
    std::string option = arguments.Get(3);
    std::string value  = arguments.Get(4);
    std::string lower  = StringUtils::ToLower(value);
    nlohmann::json message;

    if (StringUtils::EqualsIgnoreCase(option, "displayName"))
    {
        nlohmann::json displayName;
        if (!ParseComponent(context, arguments.GetRest(4), &displayName))
        {
            return;
        }

        if (displayName == team->displayName)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.team.option.name.unchanged"), true);
            return;
        }

        team->displayName = displayName;
        message           = TranslateWith("commands.team.option.name.success", {team->GetFormattedDisplayName()});
    }
    else if (StringUtils::EqualsIgnoreCase(option, "prefix") || StringUtils::EqualsIgnoreCase(option, "suffix"))
    {
        bool prefix = StringUtils::EqualsIgnoreCase(option, "prefix");
        nlohmann::json component;
        if (!ParseComponent(context, arguments.GetRest(4), &component))
        {
            return;
        }

        (prefix ? team->prefix : team->suffix) = component;
        message = TranslateWith(prefix ? "commands.team.option.prefix.success" : "commands.team.option.suffix.success", {component});
    }
    else if (StringUtils::EqualsIgnoreCase(option, "color"))
    {
        bool hex = lower.size() == 7 && lower[0] == '#' && std::all_of(lower.begin() + 1, lower.end(), isxdigit);
        if (arguments.Count() != 5 || (!hex && std::find(std::begin(kColorNames), std::end(kColorNames), lower) == std::end(kColorNames)))
        {
            context->SendSystemMessage(CommandContext::Translate("argument.color.invalid", {StringUtils::ToUtf16(value)}), true);
            return;
        }

        if (lower == team->color)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.team.option.color.unchanged"), true);
            return;
        }

        team->color = lower;
        message     = TranslateWith("commands.team.option.color.success", {team->GetFormattedDisplayName(), lower});
    }
    else if (StringUtils::EqualsIgnoreCase(option, "friendlyFire") || StringUtils::EqualsIgnoreCase(option, "seeFriendlyInvisibles"))
    {
        if (arguments.Count() != 5 || (lower != "true" && lower != "false"))
        {
            context->SendSystemMessage(CommandContext::Translate("parsing.bool.invalid", {StringUtils::ToUtf16(value)}), true);
            return;
        }

        bool friendlyFire = StringUtils::EqualsIgnoreCase(option, "friendlyFire");
        std::string key   = friendlyFire ? "commands.team.option.friendlyfire." : "commands.team.option.seeFriendlyInvisibles.";
        bool *flag        = friendlyFire ? &team->friendlyFire : &team->seeFriendlyInvisibles;
        bool enabled      = lower == "true";
        if (*flag == enabled)
        {
            context->SendSystemMessage(CommandContext::Translate(key + (enabled ? "alreadyEnabled" : "alreadyDisabled")), true);
            return;
        }

        *flag   = enabled;
        message = TranslateWith(key + (enabled ? "enabled" : "disabled"), {team->GetFormattedDisplayName()});
    }
    else if (StringUtils::EqualsIgnoreCase(option, "nametagVisibility") || StringUtils::EqualsIgnoreCase(option, "deathMessageVisibility") ||
             StringUtils::EqualsIgnoreCase(option, "collisionRule"))
    {
        bool collision           = StringUtils::EqualsIgnoreCase(option, "collisionRule");
        const char *const *names = collision ? kTeamCollisionRuleNames : kTeamVisibilityNames;
        int index                = -1;
        for (int i = 0; i < 4; i++)
        {
            if (value == names[i])
            {
                index = i;
            }
        }

        if (arguments.Count() != 5 || index < 0)
        {
            context->SendSystemMessage(CommandContext::Translate("argument.enum.invalid", {StringUtils::ToUtf16(value)}), true);
            return;
        }

        std::string key = "commands.team.option.collisionRule.";
        int *current    = (int *) &team->collisionRule;
        if (StringUtils::EqualsIgnoreCase(option, "nametagVisibility"))
        {
            key     = "commands.team.option.nametagVisibility.";
            current = (int *) &team->nameTagVisibility;
        }
        else if (!collision)
        {
            key     = "commands.team.option.deathMessageVisibility.";
            current = (int *) &team->deathMessageVisibility;
        }

        if (*current == index)
        {
            context->SendSystemMessage(CommandContext::Translate(key + "unchanged"), true);
            return;
        }

        *current = index;
        message  = TranslateWith(key + "success",
                                 {team->GetFormattedDisplayName(),
                                  CommandContext::Translate(std::string(collision ? "team.collision." : "team.visibility.") + names[index])});
    }
    else
    {
        context->SendSystemMessage(CommandContext::Translate("commands.team.usage"), true);
        return;
    }

    SendTeams(context);
    context->SendSystemMessage(message, false);
}

PlayerTeam *TeamCommand::FindTeam(CommandContext *context, const std::string &name) const
{
    PlayerTeam *team = context->GetLevel()->GetScoreboard().GetTeam(name);
    if (!team)
    {
        context->SendSystemMessage(CommandContext::Translate("team.notFound", {StringUtils::ToUtf16(name)}), true);
    }

    return team;
}

bool TeamCommand::ParseComponent(CommandContext *context, const std::string &text, nlohmann::json *component) const
{
    std::string trimmed = StringUtils::Trim(text);
    if (trimmed.empty() || (trimmed[0] != '{' && trimmed[0] != '[' && trimmed[0] != '"'))
    {
        *component = trimmed;
        return true;
    }

    try
    {
        *component = nlohmann::json::parse(trimmed);
    }
    catch (const nlohmann::json::parse_error &exception)
    {
        context->SendSystemMessage(CommandContext::Translate("argument.component.invalid", {StringUtils::ToUtf16(exception.what())}), true);
        return false;
    }

    return true;
}

bool TeamCommand::ResolveMembers(CommandContext *context, const CommandArguments &arguments, size_t index, std::vector<std::string> *members) const
{
    CommandTargetResolver resolver(context->GetPlayerList());
    for (size_t i = index; i < arguments.Count(); i++)
    {
        const std::string &text = arguments.Get(i);
        if (text.empty() || text[0] != '@')
        {
            members->push_back(text);
            continue;
        }

        std::vector<Entity *> entities;
        nlohmann::json errorMessage;
        if (!resolver.ResolveEntities(text, context, &entities, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return false;
        }

        for (size_t j = 0; j < entities.size(); j++)
        {
            members->push_back(entities[j]->GetScoreboardName());
        }
    }

    return true;
}

void TeamCommand::SendTeams(CommandContext *context) const
{
    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    context->GetLevel()->GetScoreboard().Save(tag.get());
    context->GetPlayerList()->SendToAll(std::shared_ptr<Packet>(new TeamsS2CPacket(std::move(tag))));
}

void TeamCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    std::string action = arguments.empty() ? "" : StringUtils::ToLower(arguments[0]);
    if (arguments.size() == 1)
    {
        suggestions.AddMatching("add");
        suggestions.AddMatching("empty");
        suggestions.AddMatching("join");
        suggestions.AddMatching("leave");
        suggestions.AddMatching("list");
        suggestions.AddMatching("modify");
        suggestions.AddMatching("remove");
        return;
    }

    if (arguments.size() == 2 && (action == "empty" || action == "join" || action == "list" || action == "modify" || action == "remove"))
    {
        for (const std::pair<const std::string, PlayerTeam> &entry : context->GetLevel()->GetScoreboard().GetTeams())
        {
            suggestions.AddMatching(entry.first);
        }

        return;
    }

    if (action == "leave" || (action == "join" && arguments.size() >= 3))
    {
        suggestions.AddPlayers(context->GetPlayerList());
        return;
    }

    if (action != "modify")
    {
        return;
    }

    if (arguments.size() == 3)
    {
        for (const char *option : kModifyOptions)
        {
            suggestions.AddMatching(option);
        }

        return;
    }

    if (arguments.size() != 4)
    {
        return;
    }

    std::string option = StringUtils::ToLower(arguments[2]);
    if (option == "color")
    {
        for (const char *color : kColorNames)
        {
            suggestions.AddMatching(color);
        }
    }
    else if (option == "friendlyfire" || option == "seefriendlyinvisibles")
    {
        suggestions.AddMatching("false");
        suggestions.AddMatching("true");
    }
    else if (option == "nametagvisibility" || option == "deathmessagevisibility")
    {
        for (const char *name : kTeamVisibilityNames)
        {
            suggestions.AddMatching(name);
        }
    }
    else if (option == "collisionrule")
    {
        for (const char *name : kTeamCollisionRuleNames)
        {
            suggestions.AddMatching(name);
        }
    }
}
