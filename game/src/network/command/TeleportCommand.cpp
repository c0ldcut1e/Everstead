#include "TeleportCommand.h"

#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

TeleportCommand::TeleportCommand() : Command("tp")
{}

TeleportCommand::~TeleportCommand()
{}

bool TeleportCommand::Matches(const std::string &name) const
{
    return name == "tp" || name == "teleport";
}

bool TeleportCommand::RequiresOperator() const
{
    return true;
}

void TeleportCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    CommandTargetResolver resolver(context->GetPlayerList());
    CommandPositionParser positionParser;
    std::vector<ServerPlayer *> targets;
    nlohmann::json errorMessage;
    glm::dvec3 position;
    ServerLevel *level = context->GetLevel();
    float yaw          = context->GetYaw();
    float pitch        = context->GetPitch();

    if (arguments.Count() == 2)
    {
        std::vector<ServerPlayer *> destinations;
        if (!resolver.ResolvePlayers(arguments.Get(1), context, &destinations, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        if (destinations.size() != 1)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.teleport.invalid_destination"), true);
            return;
        }

        ServerPlayer *player;
        if (!context->RequirePlayer(&player))
        {
            return;
        }

        targets.push_back(player);
        level    = (ServerLevel *) destinations[0]->GetLevel();
        position = destinations[0]->GetPosition();
        yaw      = destinations[0]->GetYaw();
        pitch    = destinations[0]->GetPitch();
    }
    else if (arguments.Count() == 4 && positionParser.IsCoordinate(arguments.Get(1)))
    {
        ServerPlayer *player;
        if (!context->RequirePlayer(&player))
        {
            return;
        }

        targets.push_back(player);
        if (!positionParser.ParseVec3(arguments.GetValues(), 1, context, &position, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }
    }
    else if (arguments.Count() == 3)
    {
        if (!resolver.ResolvePlayers(arguments.Get(1), context, &targets, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        std::vector<ServerPlayer *> destinations;
        if (!resolver.ResolvePlayers(arguments.Get(2), context, &destinations, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        if (destinations.size() != 1)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.teleport.invalid_destination"), true);
            return;
        }

        level    = (ServerLevel *) destinations[0]->GetLevel();
        position = destinations[0]->GetPosition();
        yaw      = destinations[0]->GetYaw();
        pitch    = destinations[0]->GetPitch();
    }
    else if (arguments.Count() == 5)
    {
        if (!resolver.ResolvePlayers(arguments.Get(1), context, &targets, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        if (!positionParser.ParseVec3(arguments.GetValues(), 2, context, &position, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }
    }
    else
    {
        context->SendSystemMessage(CommandContext::Translate("commands.tp.usage"), true);
        return;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        if (targets[i]->GetLevel() != level)
        {
            context->GetPlayerList()->ChangeDimension(targets[i], level->GetDimensionId(), position, 1.0f);
        }

        context->TeleportPlayer(targets[i], position, yaw, pitch);
    }

    context->SendSystemMessage(CommandContext::Translate("commands.teleport.success", {context->JoinPlayerNames(targets)}), false);
}

void TeleportCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                               std::vector<std::string> *completions) const
{
    (void) target;

    if (arguments.size() == 1 || arguments.size() == 2)
    {
        CommandSuggestions(arguments, completions).AddPlayers(context->GetPlayerList());
    }
}
