#include "LocateCommand.h"

#include <cmath>

#include <nlohmann/json.hpp>

#include "../../utils/StringUtils.h"
#include "../../world/biome/Biome.h"
#include "../../world/biome/BiomeRegistry.h"
#include "../../world/generation/BiomeSource.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"

LocateCommand::LocateCommand() : Command("locate")
{}

LocateCommand::~LocateCommand()
{}

bool LocateCommand::RequiresOperator() const
{
    return true;
}

void LocateCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 3 || StringUtils::ToLower(arguments.Get(1)) != "biome")
    {
        context->SendSystemMessage(CommandContext::Translate("commands.locate.usage"), true);
        return;
    }

    LocateBiome(context, StringUtils::ToLower(arguments.Get(2)));
}

void LocateCommand::LocateBiome(CommandContext *context, const std::string &name)
{
    const int searchRadius = 6400;
    const int searchStep   = 32;

    ResourceLocation location;
    Biome *biome = ResourceLocation::TryParse(name, &location) ? Biome::ByName(location) : nullptr;
    if (!biome)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.locate.biome.unknown", {StringUtils::ToUtf16(name)}), true);
        return;
    }

    const glm::dvec3 &position = context->GetPosition();
    int originX                = (int) floor(position.x);
    int originZ                = (int) floor(position.z);
    BiomeSource source(context->GetLevel()->GetSeed(), context->GetLevel()->GetGenSettings());

    bool found       = false;
    int64_t bestDist = 0;
    int bestX        = 0;
    int bestZ        = 0;
    int ringCount    = searchRadius / searchStep;
    for (int ring = 0; ring <= ringCount && !found; ring++)
    {
        for (int dz = -ring; dz <= ring; dz++)
        {
            bool edgeRow = dz == -ring || dz == ring;
            for (int dx = -ring; dx <= ring; dx += edgeRow ? 1 : ring * 2)
            {
                int x = originX + dx * searchStep;
                int z = originZ + dz * searchStep;
                if (source.GetBiome(x, z) != biome)
                {
                    continue;
                }

                int64_t dist = (int64_t) (x - originX) * (x - originX) + (int64_t) (z - originZ) * (z - originZ);
                if (!found || dist < bestDist)
                {
                    found    = true;
                    bestDist = dist;
                    bestX    = x;
                    bestZ    = z;
                }

                if (ring == 0)
                {
                    break;
                }
            }
        }
    }

    if (!found)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.locate.biome.not_found", {StringUtils::ToUtf16(location.ToString())}), true);
        return;
    }

    int distance = (int) floor(sqrt((double) bestDist));
    nlohmann::json coordinates;
    coordinates["text"]      = "[" + std::to_string(bestX) + ", ~, " + std::to_string(bestZ) + "]";
    coordinates["color"]     = "green";
    nlohmann::json component = CommandContext::Translate("commands.locate.biome.success", {StringUtils::ToUtf16(location.ToString())});
    component["with"].push_back(coordinates);
    component["with"].push_back(std::to_string(distance));
    context->SendChatMessage(context->GetCaller(), component.dump(), true);
}

void LocateCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                             std::vector<std::string> *completions) const
{
    (void) context;
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions(arguments, completions).AddMatching("biome");
        return;
    }

    if (arguments.size() == 2 && StringUtils::ToLower(arguments[0]) == "biome")
    {
        CommandSuggestions suggestions(arguments, completions);
        BiomeRegistry *registry = BiomeRegistry::GetInstance();
        for (uint32_t id = 0; id < registry->Size(); id++)
        {
            suggestions.AddMatching(registry->GetKey(id).ToString());
        }
    }
}
