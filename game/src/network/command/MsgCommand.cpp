#include "MsgCommand.h"

#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

MsgCommand::MsgCommand() : Command("msg")
{}

MsgCommand::~MsgCommand()
{}

void MsgCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 3)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.msg.usage"), true);
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

    std::wstring message = StringUtils::ToUtf16(arguments.JoinFrom(2, " "));
    if (StringUtils::Trim(message).empty())
    {
        context->SendSystemMessage(CommandContext::Translate("commands.msg.usage"), true);
        return;
    }

    std::wstring senderName = context->GetPlayerDisplayName(context->GetCaller());
    for (size_t i = 0; i < targets.size(); i++)
    {
        nlohmann::json incoming = CommandContext::Translate("commands.message.display.incoming", {senderName, message});
        incoming["color"]       = "gray";
        incoming["italic"]      = true;
        context->SendChatMessage(targets[i], incoming.dump(), true);

        nlohmann::json outgoing =
                CommandContext::Translate("commands.message.display.outgoing", {context->GetPlayerDisplayName(targets[i]), message});
        outgoing["color"]  = "gray";
        outgoing["italic"] = true;
        context->SendChatMessage(context->GetCaller(), outgoing.dump(), true);
    }
}

void MsgCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                          std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddPlayers(context->GetPlayerList());
    }
}
