#pragma once

#include "../../utils/Random.h"
#include "Command.h"

class GiveCommand : public Command
{
public:
    GiveCommand();
    ~GiveCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;

private:
    static constexpr int kMaxAmount     = 64;
    static constexpr int kMaxData       = 32767;
    static const int s_neverPickupDelay = 32767;
};
