#pragma once

#include <string>
#include <vector>

#include "Command.h"

class TickCommand : public Command
{
public:
    TickCommand();
    ~TickCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;

private:
    int ParseTime(const std::string &text) const;
};
