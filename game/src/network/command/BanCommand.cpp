#include "BanCommand.h"

#include "../../core/EversteadServer.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

BanCommand::BanCommand() : Command("ban")
{}

BanCommand::~BanCommand()
{}

bool BanCommand::RequiresOperator() const
{
    return true;
}

void BanCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.ban.usage"), true);
        return;
    }

    CommandTargetResolver resolver(context->GetPlayerList());
    std::vector<ServerPlayer *> targets;
    nlohmann::json errorMessage;
    if (!resolver.ResolvePlayers(arguments.Get(1), context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        if (EversteadServer::GetInstance()->settings.IsHost(targets[i]->GetPid()))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.ban.failed.host"), true);
            return;
        }

        if (targets[i] == context->GetCaller())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.ban.failed.self"), true);
            return;
        }
    }

    std::wstring names = context->JoinPlayerNames(targets);
    for (size_t i = 0; i < targets.size(); i++)
    {
        context->BanPlayer(targets[i]);
    }

    context->SendSystemMessage(CommandContext::Translate("commands.ban.success", {names}), false);
}

void BanCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                          std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddPlayers(context->GetPlayerList());
    }
}
