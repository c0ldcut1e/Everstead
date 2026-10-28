#include "SpawnLocationCommand.h"

#include <cmath>

#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"

SpawnLocationCommand::SpawnLocationCommand() : Command("spawnlocation")
{}

SpawnLocationCommand::~SpawnLocationCommand()
{}

bool SpawnLocationCommand::RequiresOperator() const
{
    return true;
}

void SpawnLocationCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() != 1 && arguments.Count() != 4)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.spawnlocation.usage"), true);
        return;
    }

    const glm::dvec3 &source = context->GetPosition();
    BlockPos pos((int) floor(source.x), (int) floor(source.y), (int) floor(source.z));
    nlohmann::json errorMessage;
    if (arguments.Count() == 4 && !CommandPositionParser().ParseBlockPos(arguments.GetValues(), 1, context, &pos, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    context->GetLevel()->SetWorldSpawn(pos);
    context->SendSystemMessage(CommandContext::Translate("commands.spawnlocation.success",
                                                         {StringUtils::ToUtf16(std::to_string(pos.x)), StringUtils::ToUtf16(std::to_string(pos.y)),
                                                          StringUtils::ToUtf16(std::to_string(pos.z))}),
                               false);
}

void SpawnLocationCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                                    std::vector<std::string> *completions) const
{
    (void) context;

    CommandSuggestions(arguments, completions).AddCoordinate(0, target);
}
