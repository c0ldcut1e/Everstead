#include "PardonCommand.h"

#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"

PardonCommand::PardonCommand() : Command("pardon")
{}

PardonCommand::~PardonCommand()
{}

bool PardonCommand::RequiresOperator() const
{
    return true;
}

void PardonCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() != 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.pardon.usage"), true);
        return;
    }

    std::wstring name = StringUtils::ToUtf16(arguments.Get(1));
    if (!context->GetPlayerList()->RemoveBanned(arguments.Get(1)))
    {
        context->SendSystemMessage(CommandContext::Translate("commands.pardon.failed", {name}), true);
        return;
    }

    context->SendSystemMessage(CommandContext::Translate("commands.pardon.success", {name}), false);
}

void PardonCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                             std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions suggestions(arguments, completions);
        const std::vector<BannedPlayer> &bannedPlayers = context->GetPlayerList()->GetBannedPlayers();
        for (size_t i = 0; i < bannedPlayers.size(); i++)
        {
            suggestions.AddMatching(bannedPlayers[i].name);
        }
    }
}
