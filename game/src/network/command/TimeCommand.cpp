#include "TimeCommand.h"

#include <cstdint>
#include <cstdlib>

#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../packet/s2c/SetTimeS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"

static const char *const s_presetNames[] = {"day", "noon", "night", "midnight"};
static const int64_t s_presetTicks[]     = {1000, 6000, 13000, 18000};
static const int s_presetCount           = 4;

TimeCommand::TimeCommand() : Command("time")
{}

TimeCommand::~TimeCommand()
{}

bool TimeCommand::RequiresOperator() const
{
    return true;
}

void TimeCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 3 || StringUtils::ToLower(arguments.Get(1)) != "set")
    {
        context->SendSystemMessage(CommandContext::Translate("commands.time.usage"), true);
        return;
    }

    std::string value = StringUtils::ToLower(arguments.Get(2));
    int64_t ticks     = -1;
    for (int i = 0; i < s_presetCount; i++)
    {
        if (value == s_presetNames[i])
        {
            ticks = s_presetTicks[i];
        }
    }

    char *end = nullptr;
    if (ticks < 0)
    {
        ticks = strtoll(value.c_str(), &end, 10);
    }
    if ((end && (end == value.c_str() || *end != '\0')) || ticks < 0)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.time.invalid"), true);
        return;
    }

    context->GetLevel()->SetWorldTime(ticks);
    context->GetPlayerList()->SendToLevel(
            context->GetLevel(),
            std::shared_ptr<Packet>(new SetTimeS2CPacket(ticks, context->GetLevel()->GetGameRules().GetBool(eGameRule_DoDaylightCycle))));
    context->SendSystemMessage(CommandContext::Translate("commands.time.set", {std::to_wstring(ticks)}), false);
}

void TimeCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) context;
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddMatching("set");
    }
    else if (arguments.size() == 2 && StringUtils::ToLower(arguments[0]) == "set")
    {
        CommandSuggestions suggestions(arguments, completions);
        for (int i = 0; i < s_presetCount; i++)
        {
            suggestions.AddMatching(s_presetNames[i]);
        }
    }
}
