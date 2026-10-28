#include "KillCommand.h"

#include "../../entity/LivingEntity.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

KillCommand::KillCommand() : Command("kill")
{}

KillCommand::~KillCommand()
{}

bool KillCommand::RequiresOperator() const
{
    return true;
}

void KillCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() > 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.kill.usage"), true);
        return;
    }

    nlohmann::json errorMessage;
    std::vector<Entity *> targets;
    CommandTargetResolver resolver(context->GetPlayerList());
    if (!resolver.ResolveEntities(arguments.Count() == 2 ? arguments.Get(1) : "@s", context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    std::wstring firstName = targets[0]->GetName();
    for (size_t i = 0; i < targets.size(); i++)
    {
        LivingEntity *living = dynamic_cast<LivingEntity *>(targets[i]);
        if (living)
        {
            living->Hurt(eDamageSource_OutOfWorld, 1000, nullptr);
        }
        else
        {
            targets[i]->GetLevel()->RemoveEntity(targets[i]);
        }
    }

    if (targets.size() == 1)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.kill.success.single", {firstName}), false);
    }
    else
    {
        context->SendSystemMessage(
                CommandContext::Translate("commands.kill.success.multiple", {StringUtils::ToUtf16(std::to_string(targets.size()))}), false);
    }
}

void KillCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        suggestions.AddPlayers(context->GetPlayerList());
        suggestions.AddMatching("@a");
        suggestions.AddMatching("@e");
        suggestions.AddMatching("@p");
        suggestions.AddMatching("@r");
        suggestions.AddMatching("@s");
    }
}
