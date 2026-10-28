#include "SpawnPointCommand.h"

#include <cmath>

#include "../../entity/player/ServerPlayer.h"
#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

SpawnPointCommand::SpawnPointCommand() : Command("spawnpoint")
{}

SpawnPointCommand::~SpawnPointCommand()
{}

bool SpawnPointCommand::RequiresOperator() const
{
    return true;
}

void SpawnPointCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() != 1 && arguments.Count() != 2 && arguments.Count() != 5)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.spawnpoint.usage"), true);
        return;
    }

    nlohmann::json errorMessage;
    std::vector<ServerPlayer *> targets;
    CommandTargetResolver resolver(context->GetPlayerList());
    if (!resolver.ResolvePlayers(arguments.Count() >= 2 ? arguments.Get(1) : "@s", context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    const glm::dvec3 &source = context->GetPosition();
    BlockPos pos((int) floor(source.x), (int) floor(source.y), (int) floor(source.z));
    if (arguments.Count() == 5 && !CommandPositionParser().ParseBlockPos(arguments.GetValues(), 2, context, &pos, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        targets[i]->SetRespawnPosition(pos, context->GetLevel()->GetDimensionId(), true);
    }

    context->SendSystemMessage(CommandContext::Translate("commands.spawnpoint.success",
                                                         {StringUtils::ToUtf16(std::to_string(pos.x)), StringUtils::ToUtf16(std::to_string(pos.y)),
                                                          StringUtils::ToUtf16(std::to_string(pos.z)), context->JoinPlayerNames(targets)}),
                               false);
}

void SpawnPointCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                                 std::vector<std::string> *completions) const
{
    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        suggestions.AddPlayers(context->GetPlayerList());
        return;
    }

    suggestions.AddCoordinate(1, target);
}
