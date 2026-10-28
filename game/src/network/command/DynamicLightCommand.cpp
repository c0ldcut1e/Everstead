#include "DynamicLightCommand.h"

#include "../../entity/player/ServerPlayer.h"
#include "../listener/PlayerConnection.h"
#include "../packet/s2c/DynamicLightMenuS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"

DynamicLightCommand::DynamicLightCommand() : Command("dynamiclight")
{}

DynamicLightCommand::~DynamicLightCommand()
{}

bool DynamicLightCommand::RequiresOperator() const
{
    return true;
}

void DynamicLightCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() > 1)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.dynamiclight.usage"), true);
        return;
    }

    ServerPlayer *player;
    if (!context->RequirePlayer(&player))
    {
        return;
    }

    player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new DynamicLightMenuS2CPacket()));
}
