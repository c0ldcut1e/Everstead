#include "KickCommand.h"

#include "../../core/EversteadServer.h"
#include "../../core/Localization.h"
#include "../../utils/StringUtils.h"
#include "../listener/PlayerConnection.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

KickCommand::KickCommand() : Command("kick")
{}

KickCommand::~KickCommand()
{}

bool KickCommand::RequiresOperator() const
{
    return true;
}

void KickCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.kick.usage"), true);
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
            context->SendSystemMessage(CommandContext::Translate("commands.kick.failed.host"), true);
            return;
        }

        if (targets[i] == context->GetCaller())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.kick.failed.self"), true);
            return;
        }
    }

    std::wstring reason = StringUtils::ToUtf16(arguments.JoinFrom(2, " "));
    for (size_t i = 0; i < targets.size(); i++)
    {
        context->SendSystemMessage(
                CommandContext::Translate("commands.kick.success", {context->GetPlayerDisplayName(targets[i]),
                                                                    reason.empty() ? Localization::GetInstance()->Get("disconnect.kicked") : reason}),
                false);
        targets[i]->GetPlayerConnection()->Disconnect(eDisconnectReason_Kicked, reason);
    }
}

void KickCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddPlayers(context->GetPlayerList());
    }
}
