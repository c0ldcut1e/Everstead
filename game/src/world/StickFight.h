#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "block/BlockPos.h"

class EversteadServer;
class ServerPlayer;

class StickFight
{
public:
    StickFight();

    void Reset(EversteadServer *server);
    void OnPlayerJoined(ServerPlayer *player);
    void OnPlayerLeft(ServerPlayer *player);
    bool OnBlockBroken(ServerPlayer *player, const BlockPos &pos);
    void OnRespawn(ServerPlayer *player);
    void Tick();
    void SendState();

private:
    struct Team
    {
        bool active;
        int bridge;
        std::string member;
        int score;
        BlockPos bed;
    };

    void ResetRound();
    void GiveKit(ServerPlayer *player);
    void BuildCenter();
    void BuildBridge(int bridge, int color, bool remove);
    void SetBlock(const BlockPos &pos, const char *name);
    void RememberLayout(ServerPlayer *player);

    EversteadServer *m_server;
    std::array<Team, 4> m_teams;
    std::unordered_set<BlockPos> m_arena;
    std::unordered_map<std::string, std::array<int, 3>> m_layouts;
    bool m_built;
};
