#include "GiveCommand.h"

#include "../../entity/Inventory.h"
#include "../../entity/item/ItemEntity.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../tag/TagParser.h"
#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

GiveCommand::GiveCommand() : Command("give")
{}

GiveCommand::~GiveCommand()
{}

bool GiveCommand::RequiresOperator() const
{
    return true;
}

void GiveCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 3)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.give.usage"), true);
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

    Item *item = context->FindItem(arguments.Get(2));
    if (!item)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.item.unknown", {StringUtils::ToUtf16(arguments.Get(2))}), true);
        return;
    }

    int amount = 1;
    int data   = 0;
    if ((arguments.Count() >= 4 && !arguments.GetInt(3, 1, kMaxAmount, &amount, &errorMessage)) ||
        (arguments.Count() >= 5 && !arguments.GetInt(4, 0, kMaxData, &data, &errorMessage)))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    std::unique_ptr<CompoundTag> tag;
    if (arguments.Count() >= 6)
    {
        TagParser parser(arguments.GetRest(5));
        tag = parser.Parse();
        if (!tag)
        {
            context->SendSystemMessage(CommandContext::Translate("argument.nbt.invalid", {std::to_wstring(parser.GetPosition())}), true);
            return;
        }
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        ItemInstance stack(item, amount, data);
        if (tag)
        {
            stack.SetTag((CompoundTag *) tag->Clone().release());
        }

        targets[i]->GetInventory()->AddItem(stack);
        context->GetPlayerList()->SendInventoryTo(targets[i]);

        if (stack.IsEmpty())
        {
            ItemEntity *fakeItem = targets[i]->ThrowItem(ItemInstance(item, 1, data));
            fakeItem->SetAge(ItemEntity::s_lifetime - 1);
            fakeItem->SetPickupDelay(s_neverPickupDelay);
            targets[i]->GetLevel()->PlaySound(
                    "random.pop", targets[i]->GetPosition(), 0.2f,
                    ((targets[i]->GetLevel()->GetRandom().NextFloat() - targets[i]->GetLevel()->GetRandom().NextFloat()) * 0.7f + 1.0f) * 2.0f);
        }
        else
        {
            targets[i]->ThrowItem(stack)->SetPickupDelay(0);
        }

        context->SendSystemMessage(
                CommandContext::Translate("commands.give.success", {StringUtils::ToUtf16(item->GetResourceLocation().ToString()),
                                                                    std::to_wstring(amount), context->GetPlayerDisplayName(targets[i])}),
                false);
    }
}

void GiveCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
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
