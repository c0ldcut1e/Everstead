#pragma once

#include "Command.h"

class TellRawCommand : public Command
{
public:
    TellRawCommand();
    ~TellRawCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;

private:
    bool IsAdvancedMessage(const std::string &message) const;
};
