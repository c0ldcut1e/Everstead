#include "StopSoundCommand.h"

#include <algorithm>

#include "../../sound/SoundRegistry.h"
#include "../../utils/StringUtils.h"
#include "../listener/PlayerConnection.h"
#include "../packet/s2c/StopSoundS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

static const char *const s_soundSources[] = {"*",       "master",  "music",  "record",  "weather", "block",
                                             "hostile", "neutral", "player", "ambient", "voice"};

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

StopSoundCommand::StopSoundCommand() : Command("stopsound")
{}

StopSoundCommand::~StopSoundCommand()
{}

bool StopSoundCommand::RequiresOperator() const
{
    return true;
}

bool StopSoundCommand::IsSource(const std::string &text)
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

void StopSoundCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    const nlohmann::json usage = CommandContext::Translate("commands.stopsound.usage");
    if (arguments.Count() < 2 || arguments.Count() > 4)
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    nlohmann::json errorMessage;
    std::vector<ServerPlayer *> targets;
    CommandTargetResolver resolver(context->GetPlayerList());
    if (!resolver.ResolvePlayers(arguments.Get(1), context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    std::string source = "*";
    if (arguments.Count() >= 3)
    {
        source = StringUtils::ToLower(arguments.Get(2));
        if (!IsSource(source))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.stopsound.unknown_source", {StringUtils::ToUtf16(source)}), true);
            return;
        }
    }

    std::string soundId;
    if (arguments.Count() == 4)
    {
        ResourceLocation eventId;
        bool known = false;
        if (ResourceLocation::TryParse(StringUtils::ToLower(arguments.Get(3)), &eventId))
        {
            for (uint32_t id = 0; id < SoundRegistry::Size() && !known; id++)
            {
                const ResourceLocation &location = SoundRegistry::ById(id)->GetResourceLocation();
                known                            = location == eventId || GetSoundEventId(location) == eventId;
            }
        }

        if (!known)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.sound.unknown", {StringUtils::ToUtf16(arguments.Get(3))}), true);
            return;
        }

        soundId = eventId.ToString();
    }

    int stopped = 0;
    for (size_t i = 0; i < targets.size(); i++)
    {
        PlayerConnection *connection = targets[i]->GetPlayerConnection();
        if (connection)
        {
            connection->Send(std::shared_ptr<Packet>(new StopSoundS2CPacket(source, soundId)));
            stopped++;
        }
    }

    std::string key = soundId.empty() ? "commands.stopsound.success.all" : "commands.stopsound.success.sound";
    if (source != "*")
    {
        key += "_source";
    }

    std::vector<std::wstring> messageArguments;
    if (!soundId.empty())
    {
        messageArguments.push_back(StringUtils::ToUtf16(soundId));
    }

    if (source != "*")
    {
        messageArguments.push_back(StringUtils::ToUtf16(source));
    }

    messageArguments.push_back(std::to_wstring(stopped));
    context->SendSystemMessage(CommandContext::Translate(key, messageArguments), false);
}

void StopSoundCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
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
        for (size_t i = 0; i < sizeof(s_soundSources) / sizeof(s_soundSources[0]); i++)
        {
            suggestions.AddMatching(s_soundSources[i]);
        }
    }
    else if (arguments.size() == 3)
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
}
