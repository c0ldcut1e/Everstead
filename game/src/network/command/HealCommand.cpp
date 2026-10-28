#include "HealCommand.h"

#include "../../entity/player/ServerPlayer.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

HealCommand::HealCommand() : Command("heal")
{}

HealCommand::~HealCommand()
{}

bool HealCommand::RequiresOperator() const
{
    return true;
}

void HealCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() > 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.heal.usage"), true);
        return;
    }

    nlohmann::json errorMessage;
    std::vector<ServerPlayer *> targets;
    CommandTargetResolver resolver(context->GetPlayerList());
    if (!resolver.ResolvePlayers(arguments.Count() == 2 ? arguments.Get(1) : "@s", context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        if (targets[i]->GetHealth() <= 0)
        {
            continue;
        }

        targets[i]->SetHealth(targets[i]->GetMaxHealth());
        targets[i]->SendHealth();
    }

    context->SendSystemMessage(CommandContext::Translate("commands.heal.success", {context->JoinPlayerNames(targets)}), false);
}

void HealCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        suggestions.AddPlayers(context->GetPlayerList());
    }
}
