#pragma once

#include "Command.h"

class DynamicLightCommand : public Command
{
public:
    DynamicLightCommand();
    ~DynamicLightCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
};
