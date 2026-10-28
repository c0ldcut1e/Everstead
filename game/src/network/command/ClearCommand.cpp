#include "ClearCommand.h"

#include <algorithm>
#include <climits>

#include "../../entity/Inventory.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

ClearCommand::ClearCommand() : Command("clear")
{}

ClearCommand::~ClearCommand()
{}

bool ClearCommand::RequiresOperator() const
{
    return true;
}

void ClearCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() > 5)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.clear.usage"), true);
        return;
    }

    CommandTargetResolver resolver(context->GetPlayerList());
    std::vector<ServerPlayer *> targets;
    nlohmann::json errorMessage;
    if (!resolver.ResolvePlayers(arguments.Count() >= 2 ? arguments.Get(1) : "@s", context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    Item *item = nullptr;
    if (arguments.Count() >= 3)
    {
        item = context->FindItem(arguments.Get(2));
        if (!item)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.item.unknown", {StringUtils::ToUtf16(arguments.Get(2))}), true);
            return;
        }
    }

    int data     = -1;
    int maxCount = -1;
    if ((arguments.Count() >= 4 && !arguments.GetInt(3, -1, INT_MAX, &data, &errorMessage)) ||
        (arguments.Count() >= 5 && !arguments.GetInt(4, -1, INT_MAX, &maxCount, &errorMessage)))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        Inventory *inventory = targets[i]->GetInventory();
        int removed          = 0;
        for (int slot = 0; slot < Inventory::s_slotCount; slot++)
        {
            ItemInstance *stack = inventory->GetSlot(slot);
            if (stack->IsEmpty() || (item && stack->GetItem() != item) || (data != -1 && stack->GetAuxValue() != data))
            {
                continue;
            }

            if (maxCount == -1)
            {
                removed += stack->GetCount();
                inventory->SetSlot(slot, ItemInstance());
                continue;
            }

            int amount = std::min(stack->GetCount(), maxCount - removed);
            removed += amount;
            if (amount == stack->GetCount())
            {
                inventory->SetSlot(slot, ItemInstance());
            }
            else
            {
                stack->SetCount(stack->GetCount() - amount);
            }
        }

        if (removed == 0 && maxCount != 0)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.clear.failure", {context->GetPlayerDisplayName(targets[i])}), true);
            continue;
        }

        context->GetPlayerList()->SendInventoryTo(targets[i]);
        context->SendSystemMessage(
                CommandContext::Translate("commands.clear.success", {context->GetPlayerDisplayName(targets[i]), std::to_wstring(removed)}), false);
    }
}

void ClearCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                            std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        suggestions.AddPlayers(context->GetPlayerList());
    }
    else if (arguments.size() == 2)
    {
        suggestions.AddItems();
    }
}
