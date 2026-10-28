#pragma once

#include <string>
#include <vector>

#include "../../core/EversteadServer.h"
#include "../../entity/player/ServerPlayer.h"
#include "Command.h"
#include "CommandContext.h"

class CommandDispatcher
{
public:
    CommandDispatcher();
    ~CommandDispatcher();

    void Execute(const std::string &message, EversteadServer *server, ServerPlayer *source);
    void Execute(const std::string &line, CommandContext *context);
    void Complete(const std::string &text, EversteadServer *server, ServerPlayer *source, const BlockPos *target,
                  std::vector<std::string> *completions) const;
    void RegisterCommand(Command *command);
    Command *FindCommand(const std::string &name) const;

private:
    std::vector<Command *> m_commands;
};
