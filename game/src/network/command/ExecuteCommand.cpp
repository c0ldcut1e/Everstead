#include "ExecuteCommand.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>

#include "../../core/ResourceLocation.h"
#include "../../entity/EntityFactory.h"
#include "../../entity/Mob.h"
#include "../../entity/projectile/Arrow.h"
#include "../../entity/projectile/ThrowableProjectile.h"
#include "../../utils/Random.h"
#include "../../utils/StringUtils.h"
#include "../../utils/math/Mth.h"
#include "../../world/DimensionId.h"
#include "../../world/biome/Biome.h"
#include "../../world/chunk/Chunk.h"
#include "CommandArguments.h"
#include "CommandDispatcher.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"
#include "EntitySelector.h"

ExecuteCommand::ExecuteCommand(CommandDispatcher *dispatcher) : Command("execute")
{
    m_dispatcher = dispatcher;
}

ExecuteCommand::~ExecuteCommand()
{}

bool ExecuteCommand::RequiresOperator() const
{
    return true;
}

void ExecuteCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    std::vector<CommandContext> contexts = {*context};
    nlohmann::json errorMessage;
    size_t index = 1;
    while (index < arguments.Count())
    {
        std::string name = StringUtils::ToLower(arguments.Get(index));
        index++;
        if (name == "run")
        {
            std::string line = arguments.GetRest(index);
            if (line.empty())
            {
                break;
            }

            for (size_t i = 0; i < contexts.size(); i++)
            {
                if (IsEntityAlive(contexts[i]))
                {
                    m_dispatcher->Execute(line, &contexts[i]);
                }
            }

            return;
        }

        if (name == "if" || name == "unless")
        {
            size_t length = 0;
            if (!GetConditionLength(arguments, index, &length, &errorMessage))
            {
                context->SendSystemMessage(errorMessage, true);
                return;
            }

            bool expected = name == "if";
            bool last     = index + length >= arguments.Count();
            std::vector<CommandContext> passed;
            for (size_t i = 0; i < contexts.size(); i++)
            {
                int count = 0;
                if (!TestCondition(arguments, index, contexts[i], &count, &errorMessage))
                {
                    context->SendSystemMessage(errorMessage, true);
                    return;
                }

                bool success = (count > 0) == expected;
                if (!last)
                {
                    if (success)
                    {
                        passed.push_back(contexts[i]);
                    }

                    continue;
                }

                std::string type = StringUtils::ToLower(arguments.Get(index));
                bool counted     = expected && (type == "entity" || type == "blocks");
                if (success)
                {
                    context->SendSystemMessage(
                            counted ? CommandContext::Translate("commands.execute.conditional.pass_count", {std::to_wstring(count)})
                                    : CommandContext::Translate("commands.execute.conditional.pass"),
                            false);
                }
                else
                {
                    context->SendSystemMessage(
                            counted ? CommandContext::Translate("commands.execute.conditional.fail_count", {std::to_wstring(count)})
                                    : CommandContext::Translate("commands.execute.conditional.fail"),
                            true);
                }
            }

            if (last)
            {
                return;
            }

            contexts = passed;
            index += length;
            continue;
        }

        std::vector<CommandContext> results;
        size_t next = index;
        for (size_t i = 0; i < contexts.size(); i++)
        {
            next = index;
            if (!ApplyModifier(name, arguments, &next, contexts[i], &results, &errorMessage))
            {
                context->SendSystemMessage(errorMessage, true);
                return;
            }
        }

        if (contexts.empty())
        {
            CommandContext probe = *context;
            if (!ApplyModifier(name, arguments, &next, probe, &results, &errorMessage))
            {
                context->SendSystemMessage(errorMessage, true);
                return;
            }

            results.clear();
        }

        contexts = results;
        index    = next;
    }

    context->SendSystemMessage(CommandContext::Translate("commands.execute.usage"), true);
}

bool ExecuteCommand::ApplyModifier(const std::string &name, const CommandArguments &arguments, size_t *index, const CommandContext &context,
                                   std::vector<CommandContext> *results, nlohmann::json *errorMessage) const
{
    CommandPositionParser positionParser;
    size_t start = *index;
    if (name == "store")
    {
        *errorMessage = CommandContext::Translate("commands.execute.store.unsupported");
        return false;
    }

    if (start >= arguments.Count())
    {
        *errorMessage = CommandContext::Translate("commands.execute.usage");
        return false;
    }

    std::string first = StringUtils::ToLower(arguments.Get(start));
    if (name == "as" || name == "at")
    {
        std::vector<Entity *> entities;
        if (!FindEntities(arguments.Get(start), context, &entities, errorMessage))
        {
            return false;
        }

        for (size_t i = 0; i < entities.size(); i++)
        {
            CommandContext result = context;
            if (name == "as")
            {
                result.SetEntity(entities[i]);
            }
            else
            {
                result.SetLevel((ServerLevel *) entities[i]->GetLevel());
                result.SetPosition(entities[i]->GetPosition());
                result.SetRotation(entities[i]->GetYaw(), entities[i]->GetPitch());
            }

            results->push_back(result);
        }

        *index = start + 1;
        return true;
    }

    if (name == "positioned")
    {
        if (first == "as")
        {
            std::vector<Entity *> entities;
            if (start + 1 >= arguments.Count() || !FindEntities(arguments.Get(start + 1), context, &entities, errorMessage))
            {
                if (start + 1 >= arguments.Count())
                {
                    *errorMessage = CommandContext::Translate("commands.execute.usage");
                }

                return false;
            }

            for (size_t i = 0; i < entities.size(); i++)
            {
                CommandContext result = context;
                result.SetPosition(entities[i]->GetPosition());
                results->push_back(result);
            }

            *index = start + 2;
            return true;
        }

        if (first == "over")
        {
            if (start + 1 >= arguments.Count())
            {
                *errorMessage = CommandContext::Translate("commands.execute.usage");
                return false;
            }

            std::string heightmap = StringUtils::ToLower(arguments.Get(start + 1));
            if (heightmap != "world_surface" && heightmap != "motion_blocking" && heightmap != "motion_blocking_no_leaves" &&
                heightmap != "ocean_floor")
            {
                *errorMessage = CommandContext::Translate("argument.enum.invalid", {StringUtils::ToUtf16(arguments.Get(start + 1))});
                return false;
            }

            glm::dvec3 position   = context.GetPosition();
            position.y            = context.GetLevel()->GetSurfaceHeight((int) floor(position.x), (int) floor(position.z)) + 1;
            CommandContext result = context;
            result.SetPosition(position);
            results->push_back(result);
            *index = start + 2;
            return true;
        }

        glm::dvec3 position;
        if (!positionParser.ParseVec3(arguments.GetValues(), start, &context, &position, errorMessage))
        {
            return false;
        }

        CommandContext result = context;
        result.SetPosition(position);
        result.SetAnchoredAtEyes(false);
        results->push_back(result);
        *index = start + 3;
        return true;
    }

    if (name == "rotated")
    {
        if (first == "as")
        {
            std::vector<Entity *> entities;
            if (start + 1 >= arguments.Count())
            {
                *errorMessage = CommandContext::Translate("commands.execute.usage");
                return false;
            }

            if (!FindEntities(arguments.Get(start + 1), context, &entities, errorMessage))
            {
                return false;
            }

            for (size_t i = 0; i < entities.size(); i++)
            {
                CommandContext result = context;
                result.SetRotation(entities[i]->GetYaw(), entities[i]->GetPitch());
                results->push_back(result);
            }

            *index = start + 2;
            return true;
        }

        float yaw;
        float pitch;
        if (!ParseRotation(arguments, start, context, &yaw, &pitch, errorMessage))
        {
            return false;
        }

        CommandContext result = context;
        result.SetRotation(yaw, pitch);
        results->push_back(result);
        *index = start + 2;
        return true;
    }

    if (name == "facing")
    {
        if (first == "entity")
        {
            std::vector<Entity *> entities;
            if (start + 2 >= arguments.Count())
            {
                *errorMessage = CommandContext::Translate("commands.execute.usage");
                return false;
            }

            std::string anchor = StringUtils::ToLower(arguments.Get(start + 2));
            if (anchor != "eyes" && anchor != "feet")
            {
                *errorMessage = CommandContext::Translate("argument.anchor.invalid", {StringUtils::ToUtf16(arguments.Get(start + 2))});
                return false;
            }

            if (!FindEntities(arguments.Get(start + 1), context, &entities, errorMessage))
            {
                return false;
            }

            for (size_t i = 0; i < entities.size(); i++)
            {
                CommandContext target = context;
                target.SetEntity(entities[i]);
                target.SetPosition(entities[i]->GetPosition());
                target.SetAnchoredAtEyes(anchor == "eyes");
                CommandContext result = context;
                result.FacePosition(target.GetAnchorPosition());
                results->push_back(result);
            }

            *index = start + 3;
            return true;
        }

        glm::dvec3 position;
        if (!positionParser.ParseVec3(arguments.GetValues(), start, &context, &position, errorMessage))
        {
            return false;
        }

        CommandContext result = context;
        result.FacePosition(position);
        results->push_back(result);
        *index = start + 3;
        return true;
    }

    if (name == "align")
    {
        bool axes[3] = {false, false, false};
        for (size_t i = 0; i < first.size(); i++)
        {
            int axis = first[i] - 'x';
            if (axis < 0 || axis > 2 || axes[axis])
            {
                *errorMessage = CommandContext::Translate("arguments.swizzle.invalid");
                return false;
            }

            axes[axis] = true;
        }

        glm::dvec3 position = context.GetPosition();
        for (int i = 0; i < 3; i++)
        {
            if (axes[i])
            {
                position[i] = floor(position[i]);
            }
        }

        CommandContext result = context;
        result.SetPosition(position);
        results->push_back(result);
        *index = start + 1;
        return true;
    }

    if (name == "anchored")
    {
        if (first != "eyes" && first != "feet")
        {
            *errorMessage = CommandContext::Translate("argument.anchor.invalid", {StringUtils::ToUtf16(arguments.Get(start))});
            return false;
        }

        CommandContext result = context;
        result.SetAnchoredAtEyes(first == "eyes");
        results->push_back(result);
        *index = start + 1;
        return true;
    }

    if (name == "in")
    {
        CommandContext result = context;
        ServerLevel *level;
        if (!ParseDimension(arguments.Get(start), &result, &level, errorMessage))
        {
            return false;
        }

        result.SetLevel(level);
        results->push_back(result);
        *index = start + 1;
        return true;
    }

    if (name == "on")
    {
        if (first != "attacker" && first != "controller" && first != "leasher" && first != "origin" && first != "owner" && first != "passengers" &&
            first != "target" && first != "vehicle")
        {
            *errorMessage = CommandContext::Translate("argument.enum.invalid", {StringUtils::ToUtf16(arguments.Get(start))});
            return false;
        }

        std::vector<Entity *> entities;
        FindRelatedEntities(first, context.GetEntity(), &entities);
        for (size_t i = 0; i < entities.size(); i++)
        {
            CommandContext result = context;
            result.SetEntity(entities[i]);
            results->push_back(result);
        }

        *index = start + 1;
        return true;
    }

    if (name == "summon")
    {
        ResourceLocation location;
        eENTITY_TYPE type;
        if (!ResourceLocation::TryParse(first, &location) || !EntityFactory::ByName(location, &type))
        {
            *errorMessage = CommandContext::Translate("commands.summon.unknown", {StringUtils::ToUtf16(arguments.Get(start))});
            return false;
        }

        std::unique_ptr<Entity> entity = EntityFactory::Create(type, context.GetLevel());
        entity->ReadSpawnData(EntityFactory::CreateSpawnData(type, &context.GetLevel()->GetRandom()));
        entity->SetPosition(context.GetPosition());
        entity->SetRotation(context.GetYaw(), context.GetPitch());
        CommandContext result = context;
        result.SetEntity(entity.get());
        context.GetLevel()->AddEntity(std::move(entity));
        results->push_back(result);
        *index = start + 1;
        return true;
    }

    *errorMessage = CommandContext::Translate("commands.execute.usage");
    return false;
}

bool ExecuteCommand::GetConditionLength(const CommandArguments &arguments, size_t index, size_t *length, nlohmann::json *errorMessage) const
{
    if (index >= arguments.Count())
    {
        *errorMessage = CommandContext::Translate("commands.execute.usage");
        return false;
    }

    std::string type = StringUtils::ToLower(arguments.Get(index));
    if (type == "block" || type == "biome")
    {
        *length = 5;
    }
    else if (type == "blocks")
    {
        *length = 11;
    }
    else if (type == "entity" || type == "dimension")
    {
        *length = 2;
    }
    else if (type == "loaded")
    {
        *length = 4;
    }
    else
    {
        *errorMessage = CommandContext::Translate("commands.execute.conditional.unsupported", {StringUtils::ToUtf16(arguments.Get(index))});
        return false;
    }

    if (index + *length > arguments.Count())
    {
        *errorMessage = CommandContext::Translate("commands.execute.usage");
        return false;
    }

    return true;
}

bool ExecuteCommand::TestCondition(const CommandArguments &arguments, size_t index, const CommandContext &context, int *count,
                                   nlohmann::json *errorMessage) const
{
    CommandPositionParser positionParser;
    std::string type   = StringUtils::ToLower(arguments.Get(index));
    ServerLevel *level = context.GetLevel();
    *count             = 0;
    if (type == "entity")
    {
        std::vector<Entity *> entities;
        if (!FindEntities(arguments.Get(index + 1), context, &entities, errorMessage))
        {
            return false;
        }

        *count = (int) entities.size();
        return true;
    }

    if (type == "dimension")
    {
        CommandContext probe = context;
        ServerLevel *dimension;
        if (!ParseDimension(arguments.Get(index + 1), &probe, &dimension, errorMessage))
        {
            return false;
        }

        *count = dimension == level ? 1 : 0;
        return true;
    }

    BlockPos pos;
    if (!positionParser.ParseBlockPos(arguments.GetValues(), index + 1, &context, &pos, errorMessage))
    {
        return false;
    }

    bool loaded = pos.y >= 0 && pos.y < Chunk::s_sizeY &&
                  level->HasChunk(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)));
    if (type == "loaded")
    {
        *count = loaded ? 1 : 0;
        return true;
    }

    if (type == "biome")
    {
        ResourceLocation location;
        Biome *biome = ResourceLocation::TryParse(StringUtils::ToLower(arguments.Get(index + 4)), &location) ? Biome::ByName(location) : nullptr;
        if (!biome)
        {
            *errorMessage = CommandContext::Translate("commands.locate.biome.unknown", {StringUtils::ToUtf16(arguments.Get(index + 4))});
            return false;
        }

        if (!loaded)
        {
            *errorMessage = CommandContext::Translate("argument.pos.unloaded");
            return false;
        }

        ChunkPos chunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ));
        Chunk *chunk = level->GetChunk(chunkPos);
        *count       = chunk->GetBiomeAt(pos.x - chunkPos.x * Chunk::s_sizeX, pos.y, pos.z - chunkPos.z * Chunk::s_sizeZ) == biome ? 1 : 0;
        return true;
    }

    if (type == "block")
    {
        Block *block = context.FindBlock(arguments.Get(index + 4));
        if (!block)
        {
            *errorMessage = CommandContext::Translate("commands.block.unknown", {StringUtils::ToUtf16(arguments.Get(index + 4))});
            return false;
        }

        if (!loaded)
        {
            *errorMessage = CommandContext::Translate("argument.pos.unloaded");
            return false;
        }

        *count = Block::ById(level->GetBlockId(pos)) == block ? 1 : 0;
        return true;
    }

    BlockPos end;
    BlockPos destination;
    if (!positionParser.ParseBlockPos(arguments.GetValues(), index + 4, &context, &end, errorMessage) ||
        !positionParser.ParseBlockPos(arguments.GetValues(), index + 7, &context, &destination, errorMessage))
    {
        return false;
    }

    std::string mode = StringUtils::ToLower(arguments.Get(index + 10));
    if (mode != "all" && mode != "masked")
    {
        *errorMessage = CommandContext::Translate("argument.enum.invalid", {StringUtils::ToUtf16(arguments.Get(index + 10))});
        return false;
    }

    BlockPos minimum(std::min(pos.x, end.x), std::min(pos.y, end.y), std::min(pos.z, end.z));
    BlockPos maximum(std::max(pos.x, end.x), std::max(pos.y, end.y), std::max(pos.z, end.z));
    int64_t volume = (int64_t) (maximum.x - minimum.x + 1) * (maximum.y - minimum.y + 1) * (maximum.z - minimum.z + 1);
    if (volume > 32768)
    {
        *errorMessage = CommandContext::Translate("commands.execute.blocks.toobig", {L"32768", std::to_wstring(volume)});
        return false;
    }

    BlockPos offset(destination.x - minimum.x, destination.y - minimum.y, destination.z - minimum.z);
    if (minimum.y < 0 || maximum.y >= Chunk::s_sizeY || minimum.y + offset.y < 0 || maximum.y + offset.y >= Chunk::s_sizeY)
    {
        *errorMessage = CommandContext::Translate("argument.pos.unloaded");
        return false;
    }

    for (int chunkX = Mth::FloorDiv(minimum.x, Chunk::s_sizeX); chunkX <= Mth::FloorDiv(maximum.x, Chunk::s_sizeX); chunkX++)
    {
        for (int chunkZ = Mth::FloorDiv(minimum.z, Chunk::s_sizeZ); chunkZ <= Mth::FloorDiv(maximum.z, Chunk::s_sizeZ); chunkZ++)
        {
            ChunkPos destinationChunk(Mth::FloorDiv(chunkX * Chunk::s_sizeX + offset.x, Chunk::s_sizeX),
                                      Mth::FloorDiv(chunkZ * Chunk::s_sizeZ + offset.z, Chunk::s_sizeZ));
            if (!level->HasChunk(ChunkPos(chunkX, chunkZ)) || !level->HasChunk(destinationChunk))
            {
                *errorMessage = CommandContext::Translate("argument.pos.unloaded");
                return false;
            }
        }
    }

    Block *air   = Block::ByName("air");
    int matching = 0;
    for (int x = minimum.x; x <= maximum.x; x++)
    {
        for (int y = minimum.y; y <= maximum.y; y++)
        {
            for (int z = minimum.z; z <= maximum.z; z++)
            {
                BlockPos source(x, y, z);
                BlockPos target(x + offset.x, y + offset.y, z + offset.z);
                uint32_t sourceId = level->GetBlockId(source);
                if (mode == "masked" && Block::ById(sourceId) == air)
                {
                    continue;
                }

                if (sourceId != level->GetBlockId(target) || level->GetBlockData(source) != level->GetBlockData(target))
                {
                    return true;
                }

                matching++;
            }
        }
    }

    *count = matching;
    return true;
}

bool ExecuteCommand::FindEntities(const std::string &text, const CommandContext &context, std::vector<Entity *> *entities,
                                  nlohmann::json *errorMessage) const
{
    entities->clear();
    if (text[0] != '@')
    {
        CommandTargetResolver(context.GetPlayerList()).ResolveEntities(text, &context, entities, nullptr);
        return true;
    }

    EntitySelector selector;
    if (!selector.Parse(text, &context, false, errorMessage))
    {
        return false;
    }

    selector.Select(context.GetPlayerList(), &context, entities, nullptr);
    return true;
}

void ExecuteCommand::FindRelatedEntities(const std::string &relation, Entity *entity, std::vector<Entity *> *entities) const
{
    Mob *mob        = dynamic_cast<Mob *>(entity);
    Entity *related = nullptr;
    if (relation == "passengers")
    {
        *entities = entity->GetPassengers();
        return;
    }

    if (relation == "vehicle")
    {
        related = entity->GetVehicle();
    }
    else if (relation == "controller")
    {
        related = entity->GetPassengers().empty() ? nullptr : entity->GetPassengers()[0];
    }
    else if (relation == "leasher" && mob)
    {
        related = mob->GetLeashHolder();
    }
    else if (relation == "target" && mob)
    {
        related = mob->GetTarget();
    }
    else if (relation == "attacker" && mob)
    {
        related = mob->GetLastHurtByMob();
    }
    else if (relation == "owner" || relation == "origin")
    {
        Arrow *arrow                    = dynamic_cast<Arrow *>(entity);
        ThrowableProjectile *projectile = dynamic_cast<ThrowableProjectile *>(entity);
        int ownerId                     = arrow ? arrow->GetOwnerId() : projectile ? projectile->GetOwnerId() : 0;
        related                         = ownerId != 0 ? entity->GetLevel()->GetEntityById(ownerId) : nullptr;
    }

    if (related)
    {
        entities->push_back(related);
    }
}

bool ExecuteCommand::ParseRotation(const CommandArguments &arguments, size_t index, const CommandContext &context, float *yaw, float *pitch,
                                   nlohmann::json *errorMessage) const
{
    if (index + 1 >= arguments.Count())
    {
        *errorMessage = CommandContext::Translate("argument.rotation.incomplete");
        return false;
    }

    double values[2];
    for (int i = 0; i < 2; i++)
    {
        const std::string &text = arguments.Get(index + i);
        bool relative           = !text.empty() && text[0] == '~';
        std::string number      = relative ? text.substr(1) : text;
        values[i]               = 0.0;
        if (!number.empty())
        {
            errno     = 0;
            char *end = nullptr;
            values[i] = strtod(number.c_str(), &end);
            if (end == number.c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(values[i]))
            {
                *errorMessage = CommandContext::Translate("argument.pos.invalid");
                return false;
            }
        }
        else if (!relative)
        {
            *errorMessage = CommandContext::Translate("argument.pos.invalid");
            return false;
        }

        // Commands take yaw with 0 facing +z and pitch with positive looking down, so we convert into entity rotation
        if (i == 0)
        {
            *yaw = relative ? context.GetYaw() + (float) values[i] : (float) values[i] + 90.0f;
        }
        else
        {
            *pitch = relative ? context.GetPitch() - (float) values[i] : (float) -values[i];
        }
    }

    return true;
}

bool ExecuteCommand::ParseDimension(const std::string &text, CommandContext *context, ServerLevel **level, nlohmann::json *errorMessage) const
{
    ResourceLocation location;
    if (ResourceLocation::TryParse(StringUtils::ToLower(text), &location) && location.GetNamespace() == ResourceLocation::kDefaultNamespace)
    {
        for (int i = 0; i < eDimensionId_Count; i++)
        {
            if (location.GetPath() == kDimensionNames[i])
            {
                *level = context->GetServer()->GetOrCreateLevel((eDIMENSION_ID) i);
                return true;
            }
        }
    }

    *errorMessage = CommandContext::Translate("argument.dimension.invalid", {StringUtils::ToUtf16(text)});
    return false;
}

bool ExecuteCommand::IsEntityAlive(const CommandContext &context) const
{
    Entity *entity                                            = context.GetEntity();
    const std::vector<std::unique_ptr<ServerPlayer>> &players = context.GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i].get() == entity)
        {
            return true;
        }
    }

    const std::vector<ServerLevel *> &levels = context.GetServer()->GetLevels();
    for (size_t i = 0; i < levels.size(); i++)
    {
        const std::vector<std::unique_ptr<Entity>> &entities = levels[i]->GetEntities();
        for (size_t j = 0; j < entities.size(); j++)
        {
            if (entities[j].get() == entity)
            {
                return true;
            }
        }
    }

    return false;
}

void ExecuteCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                              std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    std::string previous = arguments.size() >= 2 ? StringUtils::ToLower(arguments[arguments.size() - 2]) : "";
    if (previous == "in" || previous == "dimension")
    {
        for (int i = 0; i < eDimensionId_Count; i++)
        {
            suggestions.AddMatching(ResourceLocation(ResourceLocation::kDefaultNamespace, kDimensionNames[i]).ToString());
        }

        return;
    }

    if (previous == "as" || previous == "at" || previous == "entity")
    {
        suggestions.AddPlayers(context->GetPlayerList());
        return;
    }

    if (previous == "anchored")
    {
        suggestions.AddMatching("eyes");
        suggestions.AddMatching("feet");
        return;
    }

    if (previous == "on")
    {
        const char *relations[] = {"attacker", "controller", "leasher", "origin", "owner", "passengers", "target", "vehicle"};
        for (size_t i = 0; i < 8; i++)
        {
            suggestions.AddMatching(relations[i]);
        }

        return;
    }

    if (previous == "if" || previous == "unless")
    {
        const char *conditions[] = {"biome", "block", "blocks", "dimension", "entity", "loaded"};
        for (size_t i = 0; i < 6; i++)
        {
            suggestions.AddMatching(conditions[i]);
        }

        return;
    }

    const char *subcommands[] = {"align", "anchored", "as", "at", "facing", "if", "in", "on", "positioned", "rotated", "run", "summon", "unless"};
    for (size_t i = 0; i < 13; i++)
    {
        suggestions.AddMatching(subcommands[i]);
    }
}
