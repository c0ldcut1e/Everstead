#include "FillCommand.h"

#include <algorithm>

#include "../../utils/StringUtils.h"
#include "../../utils/math/Mth.h"
#include "../../world/chunk/Chunk.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"

FillCommand::FillCommand() : Command("fill")
{}

FillCommand::~FillCommand()
{}

bool FillCommand::RequiresOperator() const
{
    return true;
}

void FillCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    const nlohmann::json usage = CommandContext::Translate("commands.fill.usage");
    if (arguments.Count() < 8 || arguments.Count() > 12)
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    CommandPositionParser positionParser;
    BlockPos from;
    BlockPos to;
    nlohmann::json errorMessage;
    if (!positionParser.ParseBlockPos(arguments.GetValues(), 1, context, &from, &errorMessage) ||
        !positionParser.ParseBlockPos(arguments.GetValues(), 4, context, &to, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    Block *block = context->FindBlock(arguments.Get(7));
    if (!block)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.block.unknown", {StringUtils::ToUtf16(arguments.Get(7))}), true);
        return;
    }

    int data = 0;
    if (arguments.Count() >= 9 && !arguments.GetInt(8, 0, 15, &data, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    std::string mode = arguments.Count() >= 10 ? StringUtils::ToLower(arguments.Get(9)) : "replace";
    if (mode != "replace" && mode != "destroy" && mode != "keep" && mode != "hollow" && mode != "outline")
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    if (mode != "replace" && arguments.Count() > 10)
    {
        context->SendSystemMessage(usage, true);
        return;
    }

    Block *filter  = nullptr;
    int filterData = -1;
    if (arguments.Count() >= 11)
    {
        filter = context->FindBlock(arguments.Get(10));
        if (!filter)
        {
            context->SendSystemMessage(CommandContext::Translate("commands.block.unknown", {StringUtils::ToUtf16(arguments.Get(10))}), true);
            return;
        }

        if (arguments.Count() == 12 && !arguments.GetInt(11, 0, 15, &filterData, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }
    }

    BlockPos minimum(std::min(from.x, to.x), std::min(from.y, to.y), std::min(from.z, to.z));
    BlockPos maximum(std::max(from.x, to.x), std::max(from.y, to.y), std::max(from.z, to.z));
    int64_t volume    = (int64_t) (maximum.x - minimum.x + 1) * (maximum.y - minimum.y + 1) * (maximum.z - minimum.z + 1);
    int64_t maxVolume = context->GetLevel()->GetGameRules().GetInt(eGameRule_CommandModificationBlockLimit);
    if (volume > maxVolume)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.fill.toobig", {std::to_wstring(maxVolume), std::to_wstring(volume)}), true);
        return;
    }

    Level *level = context->GetLevel();
    bool loaded  = minimum.y >= 0 && maximum.y < Chunk::s_sizeY;
    for (int chunkX = Mth::FloorDiv(minimum.x, Chunk::s_sizeX); loaded && chunkX <= Mth::FloorDiv(maximum.x, Chunk::s_sizeX); chunkX++)
    {
        for (int chunkZ = Mth::FloorDiv(minimum.z, Chunk::s_sizeZ); loaded && chunkZ <= Mth::FloorDiv(maximum.z, Chunk::s_sizeZ); chunkZ++)
        {
            loaded = level->HasChunk(ChunkPos(chunkX, chunkZ));
        }
    }

    if (!loaded)
    {
        context->SendSystemMessage(CommandContext::Translate("argument.pos.outofworld"), true);
        return;
    }

    Block *air = Block::ByName("air");
    int filled = 0;
    for (int x = minimum.x; x <= maximum.x; x++)
    {
        for (int y = minimum.y; y <= maximum.y; y++)
        {
            for (int z = minimum.z; z <= maximum.z; z++)
            {
                BlockPos pos(x, y, z);
                Block *currentBlock  = Block::ById(level->GetBlockId(pos));
                uint16_t currentData = level->GetBlockData(pos);
                bool edge            = x == minimum.x || x == maximum.x || y == minimum.y || y == maximum.y || z == minimum.z || z == maximum.z;
                Block *placed        = block;
                uint16_t placedData  = (uint16_t) data;
                if (!edge && mode == "outline")
                {
                    continue;
                }

                if (!edge && mode == "hollow")
                {
                    placed     = air;
                    placedData = 0;
                }

                bool filtered = filter && (currentBlock != filter || (filterData >= 0 && currentData != (uint8_t) filterData));
                if (filtered || (mode == "keep" && currentBlock != air) || (currentBlock == placed && currentData == placedData))
                {
                    continue;
                }

                if (mode == "destroy" && currentBlock != air)
                {
                    context->SetBlock(pos, air, 0);
                }

                context->SetBlock(pos, placed, placedData);
                filled++;
            }
        }
    }

    if (filled == 0)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.fill.failed"), true);
        return;
    }

    context->SendSystemMessage(CommandContext::Translate("commands.fill.success", {std::to_wstring(filled)}), false);
}

void FillCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                           std::vector<std::string> *completions) const
{
    (void) context;

    CommandSuggestions suggestions(arguments, completions);
    suggestions.AddCoordinate(0, target);
    suggestions.AddCoordinate(3, target);
    if (arguments.size() == 7 || (arguments.size() == 10 && StringUtils::ToLower(arguments[8]) == "replace"))
    {
        suggestions.AddBlocks();
    }
    else if (arguments.size() == 9)
    {
        suggestions.AddMatching("replace");
        suggestions.AddMatching("destroy");
        suggestions.AddMatching("keep");
        suggestions.AddMatching("hollow");
        suggestions.AddMatching("outline");
    }
}
