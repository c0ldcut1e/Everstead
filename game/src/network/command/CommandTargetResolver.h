#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../PlayerList.h"

class CommandContext;

class CommandTargetResolver
{
public:
    explicit CommandTargetResolver(PlayerList *playerList);
    ~CommandTargetResolver();

    bool ResolvePlayers(const std::string &targetText, const CommandContext *context, std::vector<ServerPlayer *> *players,
                        nlohmann::json *errorMessage);
    bool ResolveEntities(const std::string &targetText, const CommandContext *context, std::vector<Entity *> *entities, nlohmann::json *errorMessage);

private:
    bool Select(const std::string &targetText, const CommandContext *context, bool playersOnly, std::vector<Entity *> *entities,
                nlohmann::json *errorMessage);
    bool ResolveName(const std::string &targetText, std::vector<Entity *> *entities, nlohmann::json *errorMessage) const;

    PlayerList *m_playerList;
};
