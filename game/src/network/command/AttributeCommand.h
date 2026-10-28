#pragma once

#include "Command.h"

class AttributeInstance;
class ServerPlayer;

class AttributeCommand : public Command
{
public:
    AttributeCommand();
    ~AttributeCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;

private:
    static std::wstring FormatValue(double value);
    static std::wstring GetAttributeName(const AttributeInstance *instance);
    bool ExecuteModifier(CommandContext *context, const CommandArguments &arguments, ServerPlayer *target, AttributeInstance *instance);
};
