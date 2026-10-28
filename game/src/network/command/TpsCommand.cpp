#include "TpsCommand.h"

#include <algorithm>
#include <cwchar>

#include "../../core/EversteadServer.h"
#include "../../utils/StringUtils.h"
#include "../../world/ServerLevel.h"
#include "CommandArguments.h"
#include "CommandContext.h"

TpsCommand::TpsCommand() : Command("tps")
{}

TpsCommand::~TpsCommand()
{}

void TpsCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() > 1)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.tps.usage"), true);
        return;
    }

    EversteadServer *server      = context->GetServer();
    const TickProfiler &profiler = server->GetTickProfiler();
    wchar_t tps[32];
    wchar_t average[32];
    wchar_t worst[32];
    swprintf(tps, 32, L"%.1f", server->GetTps());
    swprintf(average, 32, L"%.2f", profiler.GetAverageTickMs());
    swprintf(worst, 32, L"%.2f", profiler.GetWorstTickMs());
    context->SendSystemMessage(CommandContext::Translate("commands.tps.summary", {tps, average, worst}), false);
    SendResults(context, "commands.tps.sections", profiler.GetSectionResults());
    SendResults(context, "commands.tps.entities", profiler.GetEntityResults());

    const std::vector<ServerLevel *> &levels = server->GetLevels();
    for (size_t i = 0; i < levels.size(); i++)
    {
        ServerLevel *level = levels[i];
        context->SendSystemMessage(
                CommandContext::Translate("commands.tps.dimension",
                                          {StringUtils::ToUtf16(kDimensionNames[level->GetDimensionId()]),
                                           std::to_wstring(level->GetLoadedChunks().size()), std::to_wstring(level->GetEntities().size()),
                                           std::to_wstring(level->GetBlockEntities().size()), std::to_wstring(level->GetScheduledBlockTickCount())}),
                false);
    }
}

void TpsCommand::SendResults(CommandContext *context, const std::string &headerKey, const std::vector<TickProfiler::Result> &results)
{
    context->SendSystemMessage(CommandContext::Translate(headerKey), false);
    for (size_t i = 0; i < std::min(results.size(), (size_t) 5); i++)
    {
        wchar_t ms[32];
        wchar_t count[32];
        swprintf(ms, 32, L"%.3f", results[i].msPerTick);
        swprintf(count, 32, L"%.1f", results[i].callsPerTick);
        context->SendSystemMessage(CommandContext::Translate("commands.tps.entry", {StringUtils::ToUtf16(results[i].name), ms, count}), false);
    }
}
