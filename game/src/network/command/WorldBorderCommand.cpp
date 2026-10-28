#include "WorldBorderCommand.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cwchar>

#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../packet/s2c/WorldBorderS2CPacket.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"

static const double kMaxSize     = 59999968.0;
static const double kMaxDistance = 29999984.0;
static const int kMaxSeconds     = 1000000000;

static std::wstring FormatNumber(double value, int decimals)
{
    wchar_t buffer[64];
    swprintf(buffer, 64, L"%.*f", decimals, value);
    return buffer;
}

static bool ParseNumber(const std::string &text, double *value)
{
    errno     = 0;
    char *end = nullptr;
    double d  = strtod(text.c_str(), &end);
    if (text.empty() || end == text.c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(d))
    {
        return false;
    }

    *value = d;
    return true;
}

static bool ParseInteger(const std::string &text, int *value)
{
    errno     = 0;
    char *end = nullptr;
    long l    = strtol(text.c_str(), &end, 10);
    if (text.empty() || end == text.c_str() || *end != '\0' || errno == ERANGE || l < 0 || l > kMaxSeconds)
    {
        return false;
    }

    *value = (int) l;
    return true;
}

WorldBorderCommand::WorldBorderCommand() : Command("worldborder")
{}

WorldBorderCommand::~WorldBorderCommand()
{}

bool WorldBorderCommand::RequiresOperator() const
{
    return true;
}

void WorldBorderCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    Level *level        = context->GetLevel();
    WorldBorder *border = &level->GetWorldBorder();
    std::string action  = arguments.Count() >= 2 ? StringUtils::ToLower(arguments.Get(1)) : "";
    std::string option  = arguments.Count() >= 3 ? StringUtils::ToLower(arguments.Get(2)) : "";
    size_t count        = arguments.Count();
    nlohmann::json message;

    if (action == "get" && count == 2)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.worldborder.get", {FormatNumber(border->GetSize(), 0)}), false);
        return;
    }

    if ((action == "set" || action == "add") && (count == 3 || count == 4))
    {
        double distance = 0.0;
        int seconds     = 0;
        if (!ParseNumber(arguments.Get(2), &distance) || (count == 4 && !ParseInteger(arguments.Get(3), &seconds)))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder." + action + ".usage"), true);
            return;
        }

        double size = action == "add" ? border->GetSize() + distance : distance;
        if (size == border->GetSize())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder.set.failed.nochange"), true);
            return;
        }
        if (size < 1.0)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder.set.failed.small"), true);
            return;
        }
        if (size > kMaxSize)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder.set.failed.big", {FormatNumber(kMaxSize, 0)}), true);
            return;
        }

        if (seconds > 0)
        {
            message = CommandContext::Translate(size < border->GetSize() ? "commands.worldborder.set.shrink" : "commands.worldborder.set.grow",
                                                {FormatNumber(size, 1), std::to_wstring(seconds)});
            border->LerpSizeBetween(border->GetSize(), size, (int64_t) seconds * 20);
        }
        else
        {
            message = CommandContext::Translate("commands.worldborder.set.immediate", {FormatNumber(size, 1)});
            border->SetSize(size);
        }
    }
    else if (action == "center" && count == 4)
    {
        CommandPositionParser positionParser;
        double x = 0.0;
        double z = 0.0;
        if (!positionParser.ParseVec2(arguments.GetValues(), 2, context, &x, &z, &message))
        {
            context->SendSystemMessage(message, true);
            return;
        }
        if (fabs(x) > kMaxDistance || fabs(z) > kMaxDistance)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder.center.failed.far", {FormatNumber(kMaxDistance, 0)}), true);
            return;
        }
        if (x == border->GetCenterX() && z == border->GetCenterZ())
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder.center.failed"), true);
            return;
        }

        border->SetCenter(x, z);
        message = CommandContext::Translate("commands.worldborder.center.success", {FormatNumber(x, 2), FormatNumber(z, 2)});
    }
    else if (action == "damage" && count == 4 && (option == "amount" || option == "buffer"))
    {
        double value = 0.0;
        if (!ParseNumber(arguments.Get(3), &value) || value < 0.0)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder.damage.invalid"), true);
            return;
        }

        if (option == "amount")
        {
            if (value == border->GetDamagePerBlock())
            {
                context->SendSystemMessage(CommandContext::Translate("commands.worldborder.damage.amount.failed"), true);
                return;
            }

            border->SetDamagePerBlock(value);
            message = CommandContext::Translate("commands.worldborder.damage.amount.success", {FormatNumber(value, 2)});
        }
        else
        {
            if (value == border->GetDamageBuffer())
            {
                context->SendSystemMessage(CommandContext::Translate("commands.worldborder.damage.buffer.failed"), true);
                return;
            }

            border->SetDamageBuffer(value);
            message = CommandContext::Translate("commands.worldborder.damage.buffer.success", {FormatNumber(value, 2)});
        }
    }
    else if (action == "warning" && count == 4 && (option == "distance" || option == "time"))
    {
        int value = 0;
        if (!ParseInteger(arguments.Get(3), &value))
        {
            context->SendSystemMessage(CommandContext::Translate("commands.worldborder.warning.invalid"), true);
            return;
        }

        if (option == "distance")
        {
            if (value == border->GetWarningBlocks())
            {
                context->SendSystemMessage(CommandContext::Translate("commands.worldborder.warning.distance.failed"), true);
                return;
            }

            border->SetWarningBlocks(value);
            message = CommandContext::Translate("commands.worldborder.warning.distance.success", {std::to_wstring(value)});
        }
        else
        {
            if (value == border->GetWarningTime())
            {
                context->SendSystemMessage(CommandContext::Translate("commands.worldborder.warning.time.failed"), true);
                return;
            }

            border->SetWarningTime(value);
            message = CommandContext::Translate("commands.worldborder.warning.time.success", {std::to_wstring(value)});
        }
    }
    else
    {
        context->SendSystemMessage(CommandContext::Translate("commands.worldborder.usage"), true);
        return;
    }

    context->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new WorldBorderS2CPacket(*border)));
    context->SendSystemMessage(message, false);
}

void WorldBorderCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                                  std::vector<std::string> *completions) const
{
    (void) context;
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        suggestions.AddMatching("add");
        suggestions.AddMatching("center");
        suggestions.AddMatching("damage");
        suggestions.AddMatching("get");
        suggestions.AddMatching("set");
        suggestions.AddMatching("warning");
    }
    else if (arguments.size() == 2 && StringUtils::ToLower(arguments[0]) == "damage")
    {
        suggestions.AddMatching("amount");
        suggestions.AddMatching("buffer");
    }
    else if (arguments.size() == 2 && StringUtils::ToLower(arguments[0]) == "warning")
    {
        suggestions.AddMatching("distance");
        suggestions.AddMatching("time");
    }
    else if ((arguments.size() == 2 || arguments.size() == 3) && StringUtils::ToLower(arguments[0]) == "center")
    {
        suggestions.AddMatching("~");
    }
}
