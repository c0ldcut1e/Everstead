#include "ExperienceCommand.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "../../entity/player/ServerPlayer.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

ExperienceCommand::ExperienceCommand() : Command("experience")
{}

ExperienceCommand::~ExperienceCommand()
{}

bool ExperienceCommand::Matches(const std::string &name) const
{
    return name == "experience" || name == "xp";
}

bool ExperienceCommand::RequiresOperator() const
{
    return true;
}

void ExperienceCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    std::string action = arguments.Count() >= 2 ? arguments.Get(1) : "";
    bool query         = action == "query";
    if ((action != "add" && action != "set" && !query) || (query && arguments.Count() != 4) ||
        (!query && (arguments.Count() < 4 || arguments.Count() > 5)))
    {
        context->SendSystemMessage(CommandContext::Translate("commands.experience.usage"), true);
        return;
    }

    std::string unit = query ? arguments.Get(3) : (arguments.Count() == 5 ? arguments.Get(4) : "points");
    if (unit != "points" && unit != "levels")
    {
        context->SendSystemMessage(CommandContext::Translate("commands.experience.usage"), true);
        return;
    }

    nlohmann::json errorMessage;
    std::vector<ServerPlayer *> targets;
    CommandTargetResolver resolver(context->GetPlayerList());
    if (!resolver.ResolvePlayers(arguments.Get(2), context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    bool levels = unit == "levels";
    if (query)
    {
        if (targets.size() != 1)
        {
            context->SendSystemMessage(CommandContext::Translate("argument.entity.toomany"), true);
            return;
        }

        int value = levels ? targets[0]->GetExperienceLevel()
                           : (int) std::round(targets[0]->GetExperienceProgress() * (float) targets[0]->GetXpNeededForNextLevel());
        context->SendSystemMessage(CommandContext::Translate(levels ? "commands.experience.query.levels" : "commands.experience.query.points",
                                                             {targets[0]->GetName(), std::to_wstring(value)}),
                                   false);
        return;
    }

    bool set = action == "set";
    int amount;
    if (!arguments.GetInt(3, set ? 0 : INT_MIN, INT_MAX, &amount, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    std::vector<ServerPlayer *> changed;
    for (size_t i = 0; i < targets.size(); i++)
    {
        ServerPlayer *player = targets[i];
        if (!set)
        {
            if (levels)
            {
                player->GiveExperienceLevels(amount);
            }
            else
            {
                player->GiveExperience(amount);
            }

            changed.push_back(player);
            continue;
        }

        if (levels)
        {
            player->SetExperience(player->GetExperienceProgress(), amount, player->GetTotalExperience());
        }
        else
        {
            if (amount >= player->GetXpNeededForNextLevel())
            {
                continue;
            }

            float needed = (float) player->GetXpNeededForNextLevel();
            player->SetExperience(std::clamp((float) amount / needed, 0.0f, (needed - 1.0f) / needed), player->GetExperienceLevel(),
                                  player->GetTotalExperience());
        }

        player->SendExperience();
        changed.push_back(player);
    }

    if (changed.empty())
    {
        context->SendSystemMessage(CommandContext::Translate("commands.experience.set.points.invalid"), true);
        return;
    }

    std::string key = "commands.experience." + action + (levels ? ".levels" : ".points") + ".success.";
    if (changed.size() == 1)
    {
        context->SendSystemMessage(CommandContext::Translate(key + "single", {std::to_wstring(amount), changed[0]->GetName()}), false);
    }
    else
    {
        context->SendSystemMessage(CommandContext::Translate(key + "multiple", {std::to_wstring(amount), std::to_wstring(changed.size())}),
                                   false);
    }
}

void ExperienceCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                                 std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 2)
    {
        suggestions.AddMatching("add");
        suggestions.AddMatching("set");
        suggestions.AddMatching("query");
    }
    else if (arguments.size() == 3)
    {
        suggestions.AddPlayers(context->GetPlayerList());
    }
    else if ((arguments.size() == 4 && arguments[1] == "query") || (arguments.size() == 5 && arguments[1] != "query"))
    {
        suggestions.AddMatching("points");
        suggestions.AddMatching("levels");
    }
}
