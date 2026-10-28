#include "Scoreboard.h"

#include <memory>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"

Scoreboard::Scoreboard()
{}

PlayerTeam *Scoreboard::GetTeam(const std::string &name)
{
    auto it = m_teams.find(name);
    return it == m_teams.end() ? nullptr : &it->second;
}

PlayerTeam *Scoreboard::AddTeam(const std::string &name)
{
    return &m_teams.emplace(name, PlayerTeam(name)).first->second;
}

void Scoreboard::RemoveTeam(PlayerTeam *team)
{
    for (const std::string &member : team->members)
    {
        m_memberTeams.erase(member);
    }

    m_teams.erase(team->name);
}

bool Scoreboard::AddMember(const std::string &member, PlayerTeam *team)
{
    PlayerTeam *current = GetMemberTeam(member);
    if (current == team)
    {
        return false;
    }

    if (current)
    {
        current->members.erase(member);
    }

    team->members.insert(member);
    m_memberTeams[member] = team->name;
    return true;
}

bool Scoreboard::RemoveMember(const std::string &member)
{
    PlayerTeam *current = GetMemberTeam(member);
    if (!current)
    {
        return false;
    }

    current->members.erase(member);
    m_memberTeams.erase(member);
    return true;
}

PlayerTeam *Scoreboard::GetMemberTeam(const std::string &member)
{
    auto it = m_memberTeams.find(member);
    return it == m_memberTeams.end() ? nullptr : GetTeam(it->second);
}

const std::map<std::string, PlayerTeam> &Scoreboard::GetTeams() const
{
    return m_teams;
}

void Scoreboard::Save(CompoundTag *tag) const
{
    std::unique_ptr<ListTag> teams(new ListTag());
    for (const std::pair<const std::string, PlayerTeam> &entry : m_teams)
    {
        std::unique_ptr<CompoundTag> teamTag(new CompoundTag());
        entry.second.Save(teamTag.get());
        teams->Add(std::move(teamTag));
    }

    tag->Put("Teams", std::move(teams));
}

void Scoreboard::Load(const CompoundTag *tag)
{
    m_teams.clear();
    m_memberTeams.clear();
    ListTag *teams = tag->GetList("Teams");
    for (size_t i = 0; teams && i < teams->Size(); i++)
    {
        const CompoundTag *teamTag = (const CompoundTag *) teams->Get(i);
        PlayerTeam *team           = AddTeam(teamTag->GetString("Name"));
        team->Load(teamTag);
        for (const std::string &member : team->members)
        {
            m_memberTeams[member] = team->name;
        }
    }
}
