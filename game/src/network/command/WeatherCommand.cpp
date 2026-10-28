#include "WeatherCommand.h"

#include <cstdlib>

#include "../../utils/Random.h"
#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"

WeatherCommand::WeatherCommand() : Command("weather")
{}

WeatherCommand::~WeatherCommand()
{}

bool WeatherCommand::RequiresOperator() const
{
    return true;
}

void WeatherCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    const int maxSeconds = 1000000;

    std::string type = arguments.Count() >= 2 ? StringUtils::ToLower(arguments.Get(1)) : "";
    if (arguments.Count() < 2 || arguments.Count() > 3 || (type != "clear" && type != "rain" && type != "thunder"))
    {
        context->SendSystemMessage(CommandContext::Translate("commands.weather.usage"), true);
        return;
    }

    int ticks = context->GetLevel()->GetRandom().NextInt(300, 900) * 20;
    if (arguments.Count() == 3)
    {
        char *end   = nullptr;
        long second = strtol(arguments.Get(2).c_str(), &end, 10);
        if (end == arguments.Get(2).c_str() || *end != '\0' || second < 1 || second > maxSeconds)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.weather.invalid_duration", {std::to_wstring(maxSeconds)}), true);
            return;
        }

        ticks = (int) second * 20;
    }

    Level *level = context->GetLevel();
    if (type == "clear")
    {
        level->SetWeather(false, ticks, false, ticks);
    }
    else
    {
        level->SetWeather(true, ticks, type == "thunder", ticks);
    }

    context->SendSystemMessage(CommandContext::Translate("commands.weather.set." + type), false);
}

void WeatherCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                              std::vector<std::string> *completions) const
{
    (void) context;
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions suggestions(arguments, completions);
        suggestions.AddMatching("clear");
        suggestions.AddMatching("rain");
        suggestions.AddMatching("thunder");
    }
}
