#pragma once

#include "Command.h"

class SeedCommand : public Command
{
public:
    SeedCommand();
    ~SeedCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
};
