#include "TickCommand.h"

#include <cstdlib>
#include <cwchar>

#include "../../core/EversteadServer.h"
#include "../../core/TickProfiler.h"
#include "../../utils/StringUtils.h"
#include "../PlayerList.h"
#include "../packet/s2c/TickingStepS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"

TickCommand::TickCommand() : Command("tick")
{}

TickCommand::~TickCommand()
{}

bool TickCommand::RequiresOperator() const
{
    return true;
}

void TickCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    EversteadServer *server  = context->GetServer();
    TickRateManager *manager = server->GetTickRateManager();
    std::string action       = arguments.Count() > 1 ? StringUtils::ToLower(arguments.Get(1)) : "";
    std::string value        = arguments.Count() > 2 ? StringUtils::ToLower(arguments.Get(2)) : "";
    if (action == "query" && arguments.Count() == 2)
    {
        float averageMs = server->GetTickProfiler().GetAverageTickMs();
        float targetMs  = 1000.0f / manager->GetTickRate();
        std::string status;
        if (manager->IsFrozen())
        {
            status = "commands.tick.status.frozen";
        }
        else if (server->IsSprinting())
        {
            status = "commands.tick.status.sprinting";
        }
        else if (averageMs > targetMs)
        {
            status = "commands.tick.status.lagging";
        }
        else
        {
            status = "commands.tick.status.running";
        }

        wchar_t rate[32];
        wchar_t average[32];
        wchar_t target[32];
        swprintf(rate, 32, L"%.1f", manager->GetTickRate());
        swprintf(average, 32, L"%.1f", averageMs);
        swprintf(target, 32, L"%.1f", targetMs);
        context->SendSystemMessage(CommandContext::Translate(status), false);
        context->SendSystemMessage(
                CommandContext::Translate(server->IsSprinting() ? "commands.tick.query.rate.sprinting" : "commands.tick.query.rate.running",
                                          {rate, average, target}),
                false);
    }
    else if (action == "rate" && arguments.Count() == 3)
    {
        char *end  = nullptr;
        float rate = strtof(value.c_str(), &end);
        if (end == value.c_str() || *end != '\0' || rate < 1.0f || rate > 10000.0f)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.tick.invalid"), true);
            return;
        }

        manager->SetTickRate(rate);
        server->SendTickingState();
        wchar_t text[32];
        swprintf(text, 32, L"%.1f", rate);
        context->SendSystemMessage(CommandContext::Translate("commands.tick.rate.success", {text}), false);
    }
    else if ((action == "freeze" || action == "unfreeze") && arguments.Count() == 2)
    {
        bool frozen = action == "freeze";
        if (frozen)
        {
            server->StopSprinting();
        }

        manager->SetFrozen(frozen);
        server->SendTickingState();
        context->SendSystemMessage(CommandContext::Translate(frozen ? "commands.tick.status.frozen" : "commands.tick.status.running"), false);
    }
    else if (action == "step" && value == "stop" && arguments.Count() == 3)
    {
        if (!manager->StopStepping())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.tick.step.stop.fail"), true);
            return;
        }

        context->GetPlayerList()->SendToAll(std::shared_ptr<Packet>(new TickingStepS2CPacket(0)));
        context->SendSystemMessage(CommandContext::Translate("commands.tick.step.stop.success"), false);
    }
    else if (action == "step" && arguments.Count() <= 3)
    {
        int ticks = arguments.Count() == 3 ? ParseTime(value) : 1;
        if (ticks < 1)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.tick.invalid"), true);
            return;
        }

        if (!manager->StepGameIfPaused(ticks))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.tick.step.fail"), true);
            return;
        }

        context->GetPlayerList()->SendToAll(std::shared_ptr<Packet>(new TickingStepS2CPacket(ticks)));
        context->SendSystemMessage(CommandContext::Translate("commands.tick.step.success", {std::to_wstring(ticks)}), false);
    }
    else if (action == "sprint" && value == "stop" && arguments.Count() == 3)
    {
        if (!server->StopSprinting())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.tick.sprint.stop.fail"), true);
            return;
        }

        context->SendSystemMessage(CommandContext::Translate("commands.tick.sprint.stop.success"), false);
    }
    else if (action == "sprint" && arguments.Count() == 3)
    {
        int ticks = ParseTime(value);
        if (ticks < 1)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.tick.invalid"), true);
            return;
        }

        if (server->RequestSprint(ticks))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.tick.sprint.stop.success"), false);
        }

        context->SendSystemMessage(CommandContext::Translate("commands.tick.status.sprinting"), false);
    }
    else
    {
        context->SendSystemMessage(CommandContext::Translate("commands.tick.usage"), true);
    }
}

int TickCommand::ParseTime(const std::string &text) const
{
    char *end    = nullptr;
    double value = strtod(text.c_str(), &end);
    if (end == text.c_str() || value < 0.0)
    {
        return -1;
    }

    std::string unit = end;
    if (unit == "s")
    {
        value *= 20.0;
    }
    else if (unit == "d")
    {
        value *= 24000.0;
    }
    else if (!unit.empty() && unit != "t")
    {
        return -1;
    }

    return (int) value;
}

void TickCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) context;
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        suggestions.AddMatching("query");
        suggestions.AddMatching("rate");
        suggestions.AddMatching("freeze");
        suggestions.AddMatching("unfreeze");
        suggestions.AddMatching("step");
        suggestions.AddMatching("sprint");
    }
    else if (arguments.size() == 2)
    {
        std::string action = StringUtils::ToLower(arguments[0]);
        if (action == "rate")
        {
            suggestions.AddMatching("20");
        }
        else if (action == "step")
        {
            suggestions.AddMatching("stop");
            suggestions.AddMatching("1t");
            suggestions.AddMatching("1s");
        }
        else if (action == "sprint")
        {
            suggestions.AddMatching("stop");
            suggestions.AddMatching("60s");
            suggestions.AddMatching("1d");
        }
    }
}
