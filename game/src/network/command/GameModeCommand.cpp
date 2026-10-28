#include "GameModeCommand.h"

#include "../../core/EversteadServer.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/StringUtils.h"
#include "../PlayerList.h"
#include "../listener/PlayerConnection.h"
#include "../packet/s2c/SetGameModeS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

GameModeCommand::GameModeCommand() : Command("gamemode")
{}

GameModeCommand::~GameModeCommand()
{}

bool GameModeCommand::RequiresOperator() const
{
    return true;
}

void GameModeCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 2 || arguments.Count() > 3)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.gamemode.usage"), true);
        return;
    }

    eGAME_TYPE gameType;
    if (!GameType::Parse(arguments.Get(1), &gameType))
    {
        context->SendSystemMessage(CommandContext::Translate("commands.gamemode.unknown", {StringUtils::ToUtf16(arguments.Get(1))}), true);
        return;
    }

    nlohmann::json errorMessage;
    std::vector<ServerPlayer *> targets;
    CommandTargetResolver resolver(context->GetPlayerList());
    std::string selector = arguments.Count() == 3 ? arguments.Get(2) : "@s";
    if (!resolver.ResolvePlayers(selector, context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        targets[i]->SetGameType(gameType);
        targets[i]->GetServer()->GetPlayerList()->BroadcastPlayerFlags(targets[i]);
        PlayerConnection *connection = targets[i]->GetPlayerConnection();
        if (connection)
        {
            connection->Send(std::shared_ptr<Packet>(new SetGameModeS2CPacket(gameType, targets[i]->IsFlying())));
        }
    }

    nlohmann::json message = CommandContext::Translate("commands.gamemode.success");
    message["with"].push_back({{"translate", std::string("world.game_mode.") + GameType::GetName(gameType)}});
    message["with"].push_back(StringUtils::ToUtf8(context->JoinPlayerNames(targets)));
    context->SendSystemMessage(message, false);
}

void GameModeCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                               std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        for (int i = 0; i < eGameType_Count; i++)
        {
            suggestions.AddMatching(GameType::GetName((eGAME_TYPE) i));
        }
    }
    else if (arguments.size() == 2)
    {
        suggestions.AddPlayers(context->GetPlayerList());
    }
}
