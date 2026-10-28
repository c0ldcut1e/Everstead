#include "DifficultyCommand.h"

#include "../../core/EversteadServer.h"
#include "../../utils/StringUtils.h"
#include "../../world/ServerLevel.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"

DifficultyCommand::DifficultyCommand() : Command("difficulty")
{}

DifficultyCommand::~DifficultyCommand()
{}

bool DifficultyCommand::RequiresOperator() const
{
    return true;
}

void DifficultyCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    constexpr const char *kNames[] = {"peaceful", "easy", "normal", "hard"};

    ServerLevel *level = context->GetServer()->GetLevel();
    if (arguments.Count() == 1)
    {
        nlohmann::json message = CommandContext::Translate("commands.difficulty.query");
        message["with"].push_back({{"translate", std::string("world.difficulty.") + kNames[level->GetDifficulty()]}});
        context->SendSystemMessage(message, false);
        return;
    }

    int difficulty   = -1;
    std::string name = arguments.Count() == 2 ? StringUtils::ToLower(arguments.Get(1)) : "";
    for (int i = 0; i < 4; i++)
    {
        if (name == kNames[i] || name == std::to_string(i))
        {
            difficulty = i;
        }
    }

    if (difficulty < 0)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.difficulty.usage"), true);
        return;
    }

    level->SetDifficulty(difficulty);
    nlohmann::json message = CommandContext::Translate("commands.difficulty.success");
    message["with"].push_back({{"translate", std::string("world.difficulty.") + kNames[difficulty]}});
    context->SendSystemMessage(message, false);
}

void DifficultyCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                                 std::vector<std::string> *completions) const
{
    (void) context;
    (void) target;

    if (arguments.size() == 1)
    {
        CommandSuggestions suggestions(arguments, completions);
        suggestions.AddMatching("peaceful");
        suggestions.AddMatching("easy");
        suggestions.AddMatching("normal");
        suggestions.AddMatching("hard");
    }
}
