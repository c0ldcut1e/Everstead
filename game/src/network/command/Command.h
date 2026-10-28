#pragma once

#include <string>
#include <vector>

#include "../../world/block/BlockPos.h"

class CommandArguments;
class CommandContext;

class Command
{
public:
    explicit Command(const std::string &name);
    virtual ~Command();

    const std::string &GetName() const;
    virtual bool Matches(const std::string &name) const;
    virtual bool RequiresOperator() const;
    virtual void Execute(CommandContext *context, const CommandArguments &arguments) = 0;
    virtual void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                          std::vector<std::string> *completions) const;

private:
    std::string m_name;
};
