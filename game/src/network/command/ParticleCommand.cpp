#include "ParticleCommand.h"

#include "../../utils/StringUtils.h"
#include "../../world/particle/ParticleRegistry.h"
#include "../PlayerList.h"
#include "../packet/s2c/ParticleS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"

ParticleCommand::ParticleCommand() : Command("particle")
{}

ParticleCommand::~ParticleCommand()
{}

bool ParticleCommand::RequiresOperator() const
{
    return true;
}

void ParticleCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    const int maxCount         = 1000;
    const nlohmann::json usage = CommandContext::Translate("commands.particle.usage");
    size_t count               = arguments.Count();
    if (count < 2 || count > 10 || (count > 2 && count < 5) || (count > 5 && count < 8))
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    ResourceLocation location;
    const ParticleDefinition *definition               = nullptr;
    const std::vector<ParticleDefinition> &definitions = ParticleRegistry::GetDefinitions();
    if (ResourceLocation::TryParse(StringUtils::ToLower(arguments.Get(1)), &location))
    {
        for (size_t i = 0; i < definitions.size(); i++)
        {
            if (definitions[i].location == location && !definitions[i].terrain)
            {
                definition = &definitions[i];
                break;
            }
        }
    }

    if (!definition)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.particle.unknown", {StringUtils::ToUtf16(arguments.Get(1))}), true);
        return;
    }

    nlohmann::json errorMessage;
    CommandPositionParser positionParser;
    glm::dvec3 position = context->GetPosition();
    if (count >= 5 && !positionParser.ParseVec3(arguments.GetValues(), 2, context, &position, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    glm::dvec3 delta;
    if (count >= 8 && (!arguments.GetDouble(5, &delta.x, &errorMessage) || !arguments.GetDouble(6, &delta.y, &errorMessage) ||
                       !arguments.GetDouble(7, &delta.z, &errorMessage)))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    double speed  = 0.0;
    int particles = 1;
    if ((count >= 9 && !arguments.GetDouble(8, &speed, &errorMessage)) ||
        (count == 10 && !arguments.GetInt(9, 0, maxCount, &particles, &errorMessage)))
    {
        context->SendSystemMessage(errorMessage.is_null() ? usage : errorMessage, true);
        return;
    }

    if (speed < 0.0)
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    context->GetPlayerList()->SendToLevel(
            context->GetLevel(), std::shared_ptr<Packet>(new ParticleS2CPacket(definition->type, position, delta, (float) speed, particles)));
    context->SendSystemMessage(CommandContext::Translate("commands.particle.success", {StringUtils::ToUtf16(definition->location.ToString())}),
                               false);
}

void ParticleCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                               std::vector<std::string> *completions) const
{
    (void) context;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        const std::vector<ParticleDefinition> &definitions = ParticleRegistry::GetDefinitions();
        for (size_t i = 0; i < definitions.size(); i++)
        {
            if (!definitions[i].terrain)
            {
                suggestions.AddMatching(definitions[i].location.ToString());
            }
        }

        return;
    }

    suggestions.AddCoordinate(1, target);
}
