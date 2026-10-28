#include "SeedCommand.h"

#include <string>

#include "CommandArguments.h"
#include "CommandContext.h"

SeedCommand::SeedCommand() : Command("seed")
{}

SeedCommand::~SeedCommand()
{}

bool SeedCommand::RequiresOperator() const
{
    return true;
}

void SeedCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() > 1)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.seed.usage"), true);
        return;
    }

    std::string seed = std::to_string(context->GetLevel()->GetSeed());
    nlohmann::json seedComponent;
    seedComponent["text"]       = seed;
    seedComponent["color"]      = "green";
    seedComponent["underlined"] = true;
    seedComponent["clickEvent"] = {{"action", "copy_to_clipboard"}, {"value", seed}};

    nlohmann::json component = CommandContext::Translate("commands.seed.success");
    component["with"].push_back(seedComponent);
    context->SendChatMessage(context->GetCaller(), component.dump(), true);
}
