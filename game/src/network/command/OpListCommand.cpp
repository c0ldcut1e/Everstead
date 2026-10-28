#include "OpListCommand.h"

#include <memory>

#include "CommandArguments.h"
#include "CommandContext.h"

OpListCommand::OpListCommand() : Command("oplist")
{}

OpListCommand::~OpListCommand()
{}

void OpListCommand::Execute(CommandContext *context, const CommandArguments &)
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = context->GetPlayerList()->GetPlayers();
    std::wstring list;
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i]->IsOperator())
        {
            if (!list.empty())
            {
                list += L", ";
            }

            list += context->GetPlayerDisplayName(players[i].get());
        }
    }

    if (list.empty())
    {
        context->SendSystemMessage(CommandContext::Translate("commands.oplist.none"), false);
        return;
    }

    context->SendSystemMessage(CommandContext::Translate("commands.oplist.success", {list}), false);
}
