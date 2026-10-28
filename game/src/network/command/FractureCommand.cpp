#include "FractureCommand.h"

#include "../../entity/Inventory.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/StringUtils.h"
#include "../../world/fracture/FractureTraits.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

FractureCommand::FractureCommand() : Command("fracture")
{}

FractureCommand::~FractureCommand()
{}

bool FractureCommand::RequiresOperator() const
{
    return true;
}

void FractureCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 3)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.fracture.usage"), true);
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

    bool clear = arguments.Get(2) == "clear";
    ResourceLocation id;
    FractureTrait *trait = clear || !ResourceLocation::TryParse(arguments.Get(2), &id) ? nullptr : FractureTraits::ByLocation(id);
    if (!clear && !trait)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.fracture.unknown", {StringUtils::ToUtf16(arguments.Get(2))}), true);
        return;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        ItemInstance *item = targets[i]->GetInventory()->GetSelectedItem();
        if (clear && FractureTraits::GetCount(*item) == 0)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.fracture.notFractured", {context->GetPlayerDisplayName(targets[i])}),
                                       true);
            continue;
        }

        if (!clear && !FractureTraits::CanApply(*item, trait))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.fracture.cannotApply", {context->GetPlayerDisplayName(targets[i])}), true);
            continue;
        }

        if (clear)
        {
            FractureTraits::Clear(item);
            context->SendSystemMessage(CommandContext::Translate("commands.fracture.cleared", {context->GetPlayerDisplayName(targets[i])}), false);
        }
        else
        {
            FractureTraits::Apply(item, trait);
            context->SendSystemMessage(CommandContext::Translate("commands.fracture.success", {StringUtils::ToUtf16(trait->GetId().ToString()),
                                                                                               context->GetPlayerDisplayName(targets[i])}),
                                       false);
        }

        context->GetPlayerList()->SendInventoryTo(targets[i]);
    }
}

void FractureCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
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
        suggestions.AddMatching("clear");
        const std::vector<FractureTrait *> &traits = FractureTraits::GetTraits();
        for (size_t i = 0; i < traits.size(); i++)
        {
            suggestions.AddMatching(traits[i]->GetId().ToString());
        }
    }
}
