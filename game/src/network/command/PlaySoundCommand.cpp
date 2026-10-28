#include "PlaySoundCommand.h"

#include "../../sound/SoundRegistry.h"
#include "../../utils/Random.h"
#include "../../utils/StringUtils.h"
#include "../listener/PlayerConnection.h"
#include "../packet/s2c/SoundS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

#include <algorithm>

static const char *const s_soundSources[] = {"master"};

PlaySoundCommand::PlaySoundCommand() : Command("playsound")
{}

PlaySoundCommand::~PlaySoundCommand()
{}

bool PlaySoundCommand::RequiresOperator() const
{
    return true;
}

static ResourceLocation GetSoundEventId(const ResourceLocation &location)
{
    const std::string &path = location.GetPath();
    size_t end              = path.size();
    while (end > 0 && path[end - 1] >= '0' && path[end - 1] <= '9')
    {
        end--;
    }

    return ResourceLocation(location.GetNamespace(), path.substr(0, end));
}

bool PlaySoundCommand::IsSource(const std::string &text)
{
    for (size_t i = 0; i < sizeof(s_soundSources) / sizeof(s_soundSources[0]); i++)
    {
        if (text == s_soundSources[i])
        {
            return true;
        }
    }

    return false;
}

void PlaySoundCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    const nlohmann::json usage = CommandContext::Translate("commands.playsound.usage");
    if (arguments.Count() < 2)
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    ResourceLocation eventId;
    std::vector<std::string> variants;
    if (ResourceLocation::TryParse(StringUtils::ToLower(arguments.Get(1)), &eventId))
    {
        for (uint32_t id = 0; id < SoundRegistry::Size(); id++)
        {
            const ResourceLocation &location = SoundRegistry::ById(id)->GetResourceLocation();
            if (location == eventId || GetSoundEventId(location) == eventId)
            {
                variants.push_back(location.ToString());
            }
        }
    }

    if (variants.empty())
    {
        context->SendSystemMessage(CommandContext::Translate("commands.sound.unknown", {StringUtils::ToUtf16(arguments.Get(1))}), true);
        return;
    }

    size_t index = 2;
    if (index < arguments.Count() && IsSource(StringUtils::ToLower(arguments.Get(index))))
    {
        index++;
    }

    nlohmann::json errorMessage;
    std::vector<ServerPlayer *> targets;
    CommandPositionParser positionParser;
    if (index < arguments.Count() && !positionParser.IsCoordinate(arguments.Get(index)))
    {
        CommandTargetResolver resolver(context->GetPlayerList());
        if (!resolver.ResolvePlayers(arguments.Get(index), context, &targets, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        index++;
    }
    else
    {
        ServerPlayer *player;
        if (!context->RequirePlayer(&player))
        {
            return;
        }

        targets.push_back(player);
    }

    bool hasPosition = false;
    glm::dvec3 position;
    if (index < arguments.Count())
    {
        if (index + 3 > arguments.Count() || !positionParser.ParseVec3(arguments.GetValues(), index, context, &position, &errorMessage))
        {
            context->SendSystemMessage(errorMessage.is_null() ? usage : errorMessage, true);
            return;
        }

        hasPosition = true;
        index += 3;
    }

    double volume = 1.0;
    double pitch  = 1.0;
    if (index < arguments.Count() && !arguments.GetDouble(index++, &volume, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    if (index < arguments.Count() && !arguments.GetDouble(index++, &pitch, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    if (index < arguments.Count() || volume < 0.0 || pitch < 0.0 || pitch > 2.0)
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    int played = 0;
    for (size_t i = 0; i < targets.size(); i++)
    {
        ServerPlayer *target         = targets[i];
        PlayerConnection *connection = target->GetPlayerConnection();
        if (!connection)
        {
            continue;
        }

        glm::dvec3 soundPosition = hasPosition ? position : target->GetPosition();
        connection->Send(std::shared_ptr<Packet>(
                new SoundS2CPacket(variants[context->GetLevel()->GetRandom().NextInt(0, (int) variants.size() - 1)], (float) volume, (float) pitch,
                                   glm::vec3((float) soundPosition.x, (float) soundPosition.y, (float) soundPosition.z))));
        played++;
    }

    context->SendSystemMessage(
            CommandContext::Translate("commands.playsound.success", {StringUtils::ToUtf16(eventId.ToString()), std::to_wstring(played)}), false);
}

void PlaySoundCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                                std::vector<std::string> *completions) const
{
    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        std::vector<std::string> added;
        for (uint32_t id = 0; id < SoundRegistry::Size(); id++)
        {
            std::string eventId = GetSoundEventId(SoundRegistry::ById(id)->GetResourceLocation()).ToString();
            if (std::find(added.begin(), added.end(), eventId) == added.end())
            {
                added.push_back(eventId);
                suggestions.AddMatching(eventId);
            }
        }
    }
    else if (arguments.size() == 2)
    {
        for (size_t i = 0; i < sizeof(s_soundSources) / sizeof(s_soundSources[0]); i++)
        {
            suggestions.AddMatching(s_soundSources[i]);
        }

        suggestions.AddPlayers(context->GetPlayerList());
    }
    else
    {
        if (arguments.size() == 3 && IsSource(arguments[1]))
        {
            suggestions.AddPlayers(context->GetPlayerList());
        }

        suggestions.AddCoordinate(IsSource(arguments[1]) ? 3 : 2, target);
    }
}
