#pragma once

#include "../../world/scoreboard/PlayerTeam.h"
#include "Command.h"

class TeamCommand : public Command
{
public:
    TeamCommand();
    ~TeamCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;

private:
    PlayerTeam *FindTeam(CommandContext *context, const std::string &name) const;
    bool ParseComponent(CommandContext *context, const std::string &text, nlohmann::json *component) const;
    bool ResolveMembers(CommandContext *context, const CommandArguments &arguments, size_t index, std::vector<std::string> *members) const;
    void Modify(CommandContext *context, PlayerTeam *team, const CommandArguments &arguments);
    void SendTeams(CommandContext *context) const;
};
