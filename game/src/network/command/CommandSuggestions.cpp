#include "CommandSuggestions.h"

#include "../../core/ResourceLocation.h"
#include "../../utils/StringUtils.h"
#include "../../world/block/BlockRegistry.h"
#include "../../world/item/ItemRegistry.h"

CommandSuggestions::CommandSuggestions(const std::vector<std::string> &arguments, std::vector<std::string> *completions)
{
    m_prefix      = StringUtils::ToLower(arguments.back());
    m_index       = arguments.size() - 1;
    m_completions = completions;
}

CommandSuggestions::~CommandSuggestions()
{}

void CommandSuggestions::AddMatching(const std::string &candidate)
{
    std::string lowerCandidate = StringUtils::ToLower(candidate);
    size_t namespaceEnd        = lowerCandidate.find(':');
    bool matchesPath           = namespaceEnd != std::string::npos && lowerCandidate.substr(namespaceEnd + 1).starts_with(m_prefix);
    if (lowerCandidate.starts_with(m_prefix) || matchesPath)
    {
        m_completions->push_back(candidate);
    }
}

void CommandSuggestions::AddCoordinate(size_t firstIndex, const BlockPos *target)
{
    if (m_index < firstIndex || m_index > firstIndex + 2)
    {
        return;
    }

    if (!target)
    {
        m_completions->push_back("~");
        return;
    }

    int coordinates[3] = {target->x, target->y, target->z};
    m_completions->push_back(std::to_string(coordinates[m_index - firstIndex]));
}

void CommandSuggestions::AddPlayers(PlayerList *playerList)
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = playerList->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        AddMatching(StringUtils::ToUtf8(players[i]->GetName()));
    }
}

void CommandSuggestions::AddBlocks()
{
    BlockRegistry *registry = BlockRegistry::GetInstance();
    for (uint32_t id = 0; id < registry->Size(); id++)
    {
        AddMatching(registry->GetKey(id).ToString());
    }
}

void CommandSuggestions::AddItems()
{
    ItemRegistry *registry = ItemRegistry::GetInstance();
    for (uint32_t id = 0; id < registry->Size(); id++)
    {
        AddMatching(registry->GetKey(id).ToString());
    }
}
