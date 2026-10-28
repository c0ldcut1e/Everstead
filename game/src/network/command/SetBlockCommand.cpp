#include "SetBlockCommand.h"

#include "../../network/PlayerList.h"
#include "../../network/packet/s2c/SignDataS2CPacket.h"
#include "../../network/packet/s2c/SpawnerDataS2CPacket.h"
#include "../../tag/TagParser.h"
#include "../../utils/StringUtils.h"
#include "../../utils/math/Mth.h"
#include "../../world/blockentity/MobSpawnerBlockEntity.h"
#include "../../world/blockentity/SignBlockEntity.h"
#include "../../world/chunk/Chunk.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"

SetBlockCommand::SetBlockCommand() : Command("setblock")
{}

SetBlockCommand::~SetBlockCommand()
{}

bool SetBlockCommand::RequiresOperator() const
{
    return true;
}

void SetBlockCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 5)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.setblock.usage"), true);
        return;
    }

    CommandPositionParser positionParser;
    BlockPos pos;
    nlohmann::json errorMessage;
    if (!positionParser.ParseBlockPos(arguments.GetValues(), 1, context, &pos, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    Block *block = context->FindBlock(arguments.Get(4));
    if (!block)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.block.unknown", {StringUtils::ToUtf16(arguments.Get(4))}), true);
        return;
    }

    int data = 0;
    if (arguments.Count() >= 6 && !arguments.GetInt(5, 0, 15, &data, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    std::string mode = arguments.Count() >= 7 ? StringUtils::ToLower(arguments.Get(6)) : "replace";
    if (mode != "replace" && mode != "destroy" && mode != "keep")
    {
        context->SendSystemMessage(CommandContext::Translate("commands.setblock.usage"), true);
        return;
    }

    std::unique_ptr<CompoundTag> tag;
    if (arguments.Count() >= 8)
    {
        TagParser parser(arguments.GetRest(7));
        tag = parser.Parse();
        if (!tag)
        {
            context->SendSystemMessage(CommandContext::Translate("argument.nbt.invalid", {std::to_wstring(parser.GetPosition())}), true);
            return;
        }
    }

    Level *level = context->GetLevel();
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY ||
        !level->HasChunk(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ))))
    {
        context->SendSystemMessage(CommandContext::Translate("argument.pos.outofworld"), true);
        return;
    }

    Block *air          = Block::ByName("air");
    Block *currentBlock = Block::ById(level->GetBlockId(pos));
    bool unchanged      = currentBlock == block && level->GetBlockData(pos) == (uint16_t) data;
    if ((unchanged && !tag) || (mode == "keep" && currentBlock != air))
    {
        context->SendSystemMessage(CommandContext::Translate("commands.setblock.failed"), true);
        return;
    }

    if (mode == "destroy" && currentBlock != air)
    {
        context->SetBlock(pos, air, 0);
    }

    context->SetBlock(pos, block, (uint16_t) data);
    BlockEntity *blockEntity = level->GetBlockEntity(pos);
    if (tag && blockEntity)
    {
        context->FlushBlockChanges();
        blockEntity->Load(tag.get());
        MobSpawnerBlockEntity *spawner = dynamic_cast<MobSpawnerBlockEntity *>(blockEntity);
        if (spawner)
        {
            context->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new SpawnerDataS2CPacket(pos, spawner->GetEntityType())));
        }
        else if (dynamic_cast<SignBlockEntity *>(blockEntity))
        {
            std::unique_ptr<CompoundTag> signTag(new CompoundTag());
            blockEntity->Save(signTag.get());
            context->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new SignDataS2CPacket(pos, std::move(signTag))));
        }
    }

    context->SendSystemMessage(CommandContext::Translate("commands.setblock.success"), false);
}

void SetBlockCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                               std::vector<std::string> *completions) const
{
    (void) context;

    CommandSuggestions suggestions(arguments, completions);
    suggestions.AddCoordinate(0, target);
    if (arguments.size() == 4)
    {
        suggestions.AddBlocks();
    }
    else if (arguments.size() == 6)
    {
        suggestions.AddMatching("replace");
        suggestions.AddMatching("destroy");
        suggestions.AddMatching("keep");
    }
}
