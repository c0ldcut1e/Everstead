#pragma once

#include <string>
#include <vector>

#include "../../core/TickProfiler.h"
#include "Command.h"

class TpsCommand : public Command
{
public:
    TpsCommand();
    ~TpsCommand() override;

    void Execute(CommandContext *context, const CommandArguments &arguments) override;

private:
    void SendResults(CommandContext *context, const std::string &headerKey, const std::vector<TickProfiler::Result> &results);
};
