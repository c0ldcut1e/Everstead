#pragma once

#include <map>
#include <string>

#include "PlayerTeam.h"

class CompoundTag;

class Scoreboard
{
public:
    Scoreboard();

    PlayerTeam *GetTeam(const std::string &name);
    PlayerTeam *AddTeam(const std::string &name);
    void RemoveTeam(PlayerTeam *team);
    bool AddMember(const std::string &member, PlayerTeam *team);
    bool RemoveMember(const std::string &member);
    PlayerTeam *GetMemberTeam(const std::string &member);
    const std::map<std::string, PlayerTeam> &GetTeams() const;

    void Save(CompoundTag *tag) const;
    void Load(const CompoundTag *tag);

private:
    std::map<std::string, PlayerTeam> m_teams;
    std::map<std::string, std::string> m_memberTeams;
};
