#include "OpCommand.h"

#include <algorithm>

#include "../../core/EversteadServer.h"
#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

OpCommand::OpCommand() : Command("op")
{}

OpCommand::~OpCommand()
{}

bool OpCommand::RequiresOperator() const
{
    return true;
}

void OpCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.op.usage"), true);
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

    ServerSettings &settings = EversteadServer::GetInstance()->settings;
    for (size_t i = 0; i < targets.size(); i++)
    {
        targets[i]->SetOperator(true);
        if (!settings.IsOperator(targets[i]->GetPid()))
        {
            settings.operators.push_back({targets[i]->GetPid(), StringUtils::ToUtf8(targets[i]->GetName())});
        }

        context->GetPlayerList()->BroadcastPlayerFlags(targets[i]);
        context->SendChatMessage(targets[i], CommandContext::Translate("player.operator_granted").dump(), true);
    }

    context->SendSystemMessage(CommandContext::Translate("commands.op.success", {context->JoinPlayerNames(targets)}), false);
}

void OpCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                         std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddPlayers(context->GetPlayerList());
    }
}
