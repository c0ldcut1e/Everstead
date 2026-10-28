#pragma once

#include "Command.h"

class TeleportCommand : public Command
{
public:
    TeleportCommand();
    ~TeleportCommand() override;

    bool Matches(const std::string &name) const override;
    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;
};
