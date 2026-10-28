#include "Command.h"

Command::Command(const std::string &name)
{
    m_name = name;
}

Command::~Command()
{}

const std::string &Command::GetName() const
{
    return m_name;
}

bool Command::Matches(const std::string &name) const
{
    return m_name == name;
}

bool Command::RequiresOperator() const
{
    return false;
}

void Command::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                       std::vector<std::string> *completions) const
{
    (void) context;
    (void) arguments;
    (void) target;
    (void) completions;
}
