#include "CommandDispatcher.h"

#include "../../core/GameInfo.h"
#include "../../utils/StringUtils.h"
#include "AttributeCommand.h"
#include "BanCommand.h"
#include "ClearCommand.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "DeopCommand.h"
#include "DifficultyCommand.h"
#include "DynamicLightCommand.h"
#include "ExecuteCommand.h"
#include "FillCommand.h"
#include "FractureCommand.h"
#include "GameModeCommand.h"
#include "GameRuleCommand.h"
#include "GiveCommand.h"
#include "HealCommand.h"
#include "KickCommand.h"
#include "KillCommand.h"
#include "LocateCommand.h"
#include "MsgCommand.h"
#include "OpCommand.h"
#include "OpListCommand.h"
#include "PardonCommand.h"
#include "ParticleCommand.h"
#include "PlaySoundCommand.h"
#include "SeedCommand.h"
#include "SetBlockCommand.h"
#include "SpawnLocationCommand.h"
#include "SpawnPointCommand.h"
#include "StopSoundCommand.h"
#include "SummonCommand.h"
#include "TeamCommand.h"
#include "TeleportCommand.h"
#include "TellRawCommand.h"
#include "TickCommand.h"
#include "TimeCommand.h"
#include "TpsCommand.h"
#include "WeatherCommand.h"
#include "WorldBorderCommand.h"
#include "ExperienceCommand.h"

CommandDispatcher::CommandDispatcher()
{
    m_commands = {};
    RegisterCommand(new BanCommand());
    RegisterCommand(new KickCommand());
    if (kStickFight)
    {
        return;
    }

    RegisterCommand(new TellRawCommand());
    RegisterCommand(new MsgCommand());
    RegisterCommand(new TeleportCommand());
    RegisterCommand(new PardonCommand());
    RegisterCommand(new TimeCommand());
    RegisterCommand(new TickCommand());
    RegisterCommand(new OpCommand());
    RegisterCommand(new DeopCommand());
    RegisterCommand(new OpListCommand());
    RegisterCommand(new SetBlockCommand());
    RegisterCommand(new FillCommand());
    RegisterCommand(new GiveCommand());
    RegisterCommand(new FractureCommand());
    RegisterCommand(new ClearCommand());
    RegisterCommand(new AttributeCommand());
    RegisterCommand(new GameRuleCommand());
    RegisterCommand(new LocateCommand());
    RegisterCommand(new WeatherCommand());
    RegisterCommand(new DifficultyCommand());
    RegisterCommand(new TeamCommand());
    RegisterCommand(new WorldBorderCommand());
    RegisterCommand(new SpawnLocationCommand());
    RegisterCommand(new SpawnPointCommand());
    RegisterCommand(new ParticleCommand());
    RegisterCommand(new SummonCommand());
    RegisterCommand(new PlaySoundCommand());
    RegisterCommand(new StopSoundCommand());
    RegisterCommand(new GameModeCommand());
    RegisterCommand(new HealCommand());
    RegisterCommand(new DynamicLightCommand());
    RegisterCommand(new ExperienceCommand());
    RegisterCommand(new KillCommand());
    RegisterCommand(new SeedCommand());
    RegisterCommand(new TpsCommand());
    RegisterCommand(new ExecuteCommand(this));
}

CommandDispatcher::~CommandDispatcher()
{
    for (size_t i = 0; i < m_commands.size(); i++)
    {
        delete m_commands[i];
    }

    m_commands.clear();
}

void CommandDispatcher::Execute(const std::string &message, EversteadServer *server, ServerPlayer *source)
{
    CommandContext context(server, source);
    Execute(StringUtils::Trim(message.substr(1)), &context);
}

void CommandDispatcher::Execute(const std::string &line, CommandContext *context)
{
    CommandArguments arguments;
    nlohmann::json parseError;
    if (!arguments.Parse(line, &parseError))
    {
        context->SendSystemMessage(parseError, true);
        return;
    }

    if (arguments.Count() == 0)
    {
        context->SendSystemMessage(CommandContext::Translate("command.unknown.command"), true);
        return;
    }

    Command *command = FindCommand(StringUtils::ToLower(arguments.Get(0)));
    if (!command)
    {
        context->SendSystemMessage(CommandContext::Translate("command.unknown.command"), true);
        return;
    }

    if (command->RequiresOperator() && !context->GetCaller()->IsOperator())
    {
        context->SendSystemMessage(CommandContext::Translate("commands.permission"), true);
        return;
    }

    context->SetAdminCommand(command->RequiresOperator());
    command->Execute(context, arguments);
    context->FlushBlockChanges();
}

void CommandDispatcher::Complete(const std::string &text, EversteadServer *server, ServerPlayer *source, const BlockPos *target,
                                 std::vector<std::string> *completions) const
{
    std::vector<std::string> words = StringUtils::Split(text.substr(1), ' ');
    if (words.size() == 1)
    {
        CommandSuggestions suggestions(words, completions);
        for (size_t i = 0; i < m_commands.size(); i++)
        {
            if (!m_commands[i]->RequiresOperator() || source->IsOperator())
            {
                suggestions.AddMatching(m_commands[i]->GetName());
            }
        }

        for (size_t i = 0; i < completions->size(); i++)
        {
            (*completions)[i] = "/" + (*completions)[i];
        }

        return;
    }

    Command *command = FindCommand(StringUtils::ToLower(words[0]));
    if (!command || (command->RequiresOperator() && !source->IsOperator()))
    {
        return;
    }

    CommandContext context(server, source);
    command->Complete(&context, std::vector<std::string>(words.begin() + 1, words.end()), target, completions);
}

void CommandDispatcher::RegisterCommand(Command *command)
{
    m_commands.push_back(command);
}

Command *CommandDispatcher::FindCommand(const std::string &name) const
{
    for (size_t i = 0; i < m_commands.size(); i++)
    {
        if (m_commands[i]->Matches(name))
        {
            return m_commands[i];
        }
    }

    return nullptr;
}
