#pragma once

#include "Command.h"

class OpListCommand : public Command
{
public:
    OpListCommand();
    ~OpListCommand() override;

    void Execute(CommandContext *context, const CommandArguments &arguments) override;
};
