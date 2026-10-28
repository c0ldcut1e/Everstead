#include "CommandTargetResolver.h"

#include "../../entity/player/ServerPlayer.h"
#include "../../utils/StringUtils.h"
#include "CommandContext.h"
#include "EntitySelector.h"

CommandTargetResolver::CommandTargetResolver(PlayerList *playerList)
{
    m_playerList = playerList;
}

CommandTargetResolver::~CommandTargetResolver()
{}

bool CommandTargetResolver::ResolvePlayers(const std::string &targetText, const CommandContext *context, std::vector<ServerPlayer *> *players,
                                           nlohmann::json *errorMessage)
{
    players->clear();
    std::vector<Entity *> entities;
    if (!Select(targetText, context, true, &entities, errorMessage))
    {
        return false;
    }

    for (size_t i = 0; i < entities.size(); i++)
    {
        players->push_back((ServerPlayer *) entities[i]);
    }

    return true;
}

bool CommandTargetResolver::ResolveEntities(const std::string &targetText, const CommandContext *context, std::vector<Entity *> *entities,
                                            nlohmann::json *errorMessage)
{
    return Select(targetText, context, false, entities, errorMessage);
}

bool CommandTargetResolver::Select(const std::string &targetText, const CommandContext *context, bool playersOnly, std::vector<Entity *> *entities,
                                   nlohmann::json *errorMessage)
{
    entities->clear();
    if (targetText.empty())
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.player.expected");
        }

        return false;
    }

    if (targetText[0] != '@')
    {
        return ResolveName(targetText, entities, errorMessage);
    }

    EntitySelector selector;
    return selector.Parse(targetText, context, playersOnly, errorMessage) && selector.Select(m_playerList, context, entities, errorMessage);
}

bool CommandTargetResolver::ResolveName(const std::string &targetText, std::vector<Entity *> *entities, nlohmann::json *errorMessage) const
{
    std::wstring targetName                                         = StringUtils::ToLower(StringUtils::ToUtf16(targetText));
    const std::vector<std::unique_ptr<ServerPlayer>> &serverPlayers = m_playerList->GetPlayers();
    for (size_t i = 0; i < serverPlayers.size(); i++)
    {
        if (StringUtils::ToLower(serverPlayers[i]->GetName()) == targetName)
        {
            entities->push_back(serverPlayers[i].get());
            return true;
        }
    }

    if (errorMessage)
    {
        *errorMessage = CommandContext::Translate("argument.player.unknown");
    }

    return false;
}
