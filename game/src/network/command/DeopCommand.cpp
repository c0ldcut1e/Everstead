#include "DeopCommand.h"

#include <algorithm>

#include "../../core/EversteadServer.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

DeopCommand::DeopCommand() : Command("deop")
{}

DeopCommand::~DeopCommand()
{}

bool DeopCommand::RequiresOperator() const
{
    return true;
}

void DeopCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.deop.usage"), true);
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
        if (EversteadServer::GetInstance()->settings.IsHost(targets[i]->GetPid()) || targets[i]->GetName() == L"Server")
        {
            context->SendSystemMessage(CommandContext::Translate("commands.deop.failed.host"), true);
            return;
        }
    }

    std::vector<ServerOperator> &operators = EversteadServer::GetInstance()->settings.operators;
    for (size_t i = 0; i < targets.size(); i++)
    {
        targets[i]->SetOperator(false);
        for (size_t j = operators.size(); j-- > 0;)
        {
            if (operators[j].pid == targets[i]->GetPid())
            {
                operators.erase(operators.begin() + j);
            }
        }

        context->GetPlayerList()->BroadcastPlayerFlags(targets[i]);
        context->SendChatMessage(targets[i], CommandContext::Translate("player.operator_revoked").dump(), true);
    }

    context->SendSystemMessage(CommandContext::Translate("commands.deop.success", {context->JoinPlayerNames(targets)}), false);
}

void DeopCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddPlayers(context->GetPlayerList());
    }
}
