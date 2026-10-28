#include "GameRuleCommand.h"

#include "../../utils/StringUtils.h"
#include "../../world/ServerLevel.h"
#include "../packet/s2c/SetTimeS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"

GameRuleCommand::GameRuleCommand() : Command("gamerule")
{}

GameRuleCommand::~GameRuleCommand()
{}

bool GameRuleCommand::RequiresOperator() const
{
    return true;
}

void GameRuleCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    eGAME_RULE rule;
    if (arguments.Count() < 2 || arguments.Count() > 3 || !GameRules::FindByName(arguments.Get(1), &rule))
    {
        context->SendSystemMessage(CommandContext::Translate("commands.gamerule.usage"), true);
        return;
    }

    GameRules &gameRules = context->GetLevel()->GetGameRules();
    std::wstring name    = StringUtils::ToUtf16(GameRules::GetName(rule));
    if (arguments.Count() == 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.gamerule.query", {name, StringUtils::ToUtf16(gameRules.ToString(rule))}),
                                   false);
        return;
    }

    int value = 0;
    nlohmann::json errorMessage;
    if (!gameRules.Parse(rule, arguments.Get(2), &value, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    gameRules.Set(rule, value);
    if (rule == eGameRule_DoDaylightCycle)
    {
        const std::vector<ServerLevel *> &levels = context->GetServer()->GetLevels();
        for (size_t i = 0; i < levels.size(); i++)
        {
            context->GetPlayerList()->SendToLevel(
                    levels[i], std::shared_ptr<Packet>(new SetTimeS2CPacket(levels[i]->GetWorldProvider().GetWorldTime(), gameRules.GetBool(rule))));
        }
    }

    context->SendSystemMessage(CommandContext::Translate("commands.gamerule.set", {name, StringUtils::ToUtf16(gameRules.ToString(rule))}), false);
}

void GameRuleCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                               std::vector<std::string> *completions) const
{
    (void) context;
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    eGAME_RULE rule;
    if (arguments.size() == 1)
    {
        for (int i = 0; i < eGameRule_Count; i++)
        {
            suggestions.AddMatching(GameRules::GetName((eGAME_RULE) i));
        }
    }
    else if (arguments.size() == 2 && GameRules::FindByName(arguments[0], &rule) && GameRules::IsBoolean(rule))
    {
        suggestions.AddMatching("true");
        suggestions.AddMatching("false");
    }
}
