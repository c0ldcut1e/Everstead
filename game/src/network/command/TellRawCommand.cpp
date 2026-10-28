#include "TellRawCommand.h"

#include <nlohmann/json.hpp>

#include "../../utils/StringUtils.h"
#include "../packet/s2c/ChatS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

TellRawCommand::TellRawCommand() : Command("tellraw")
{}

TellRawCommand::~TellRawCommand()
{}

bool TellRawCommand::RequiresOperator() const
{
    return true;
}

void TellRawCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 3)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.tellraw.usage"), true);
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

    std::string rawMessage = arguments.GetRest(2);
    std::string message    = arguments.Count() == 3 ? arguments.Get(2) : rawMessage;
    bool advanced          = IsAdvancedMessage(rawMessage);
    if (advanced)
    {
        try
        {
            const auto parsedMessage = nlohmann::json::parse(rawMessage);
            static_cast<void>(parsedMessage);
        }
        catch (const nlohmann::json::parse_error &exception)
        {
            context->SendSystemMessage(CommandContext::Translate("argument.component.invalid", {StringUtils::ToUtf16(exception.what())}), true);
            return;
        }

        message = rawMessage;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        context->SendChatMessage(targets[i], message, advanced);
    }
}

bool TellRawCommand::IsAdvancedMessage(const std::string &message) const
{
    std::string trimmed = StringUtils::Trim(message);
    return !trimmed.empty() && (trimmed[0] == '{' || trimmed[0] == '[');
}

void TellRawCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                              std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddPlayers(context->GetPlayerList());
    }
}
