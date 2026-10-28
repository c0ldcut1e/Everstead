#pragma once

#include "Command.h"

class LocateCommand : public Command
{
public:
    LocateCommand();
    ~LocateCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;

private:
    void LocateBiome(CommandContext *context, const std::string &name);
};
