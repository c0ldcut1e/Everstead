#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../../world/block/BlockPos.h"
#include "../PlayerList.h"

class CommandSuggestions
{
public:
    CommandSuggestions(const std::vector<std::string> &arguments, std::vector<std::string> *completions);
    ~CommandSuggestions();

    void AddMatching(const std::string &candidate);
    void AddCoordinate(size_t firstIndex, const BlockPos *target);
    void AddPlayers(PlayerList *playerList);
    void AddBlocks();
    void AddItems();

private:
    std::string m_prefix;
    size_t m_index;
    std::vector<std::string> *m_completions;
};
