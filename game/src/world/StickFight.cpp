#include "StickFight.h"

#include <cmath>
#include <memory>
#include <vector>

#include "../core/EversteadServer.h"
#include "../entity/Inventory.h"
#include "../entity/player/ServerPlayer.h"
#include "../network/PlayerList.h"
#include "../network/listener/PlayerConnection.h"
#include "../network/packet/s2c/StickFightScoresS2CPacket.h"
#include "../network/packet/s2c/TeamsS2CPacket.h"
#include "../tag/CompoundTag.h"
#include "../utils/Random.h"
#include "ServerLevel.h"
#include "block/Block.h"
#include "chunk/ChunkPos.h"
#include "item/Item.h"
#include "item/ItemInstance.h"
#include "scoreboard/Scoreboard.h"

#include <glm/trigonometric.hpp>

constexpr const char *kStickFightColors[] = {"blue", "red", "green", "yellow"};
constexpr int kStickFightDirX[]           = {1, -1, 0, 0};
constexpr int kStickFightDirZ[]           = {0, 0, 1, -1};
constexpr const char *kStickFightKit[]    = {"stick", "iron_pickaxe", "cut_sandstone"};
constexpr int kStickFightArenaY           = 64;
constexpr int kStickFightCenterRadius     = 0;
constexpr int kStickFightBridgeLength     = 24;

StickFight::StickFight()
{
    Reset(nullptr);
}

void StickFight::Reset(EversteadServer *server)
{
    m_server = server;
    m_built  = false;
    m_arena.clear();
    m_layouts.clear();
    for (size_t i = 0; i < m_teams.size(); i++)
    {
        m_teams[i] = {false, -1, "", 0, BlockPos(0, 0, 0)};
    }
}

void StickFight::OnPlayerJoined(ServerPlayer *player)
{
    if (!m_built)
    {
        BuildCenter();
        m_built = true;
    }

    std::vector<int> freeColors;
    bool bridgeUsed[4] = {};
    for (size_t i = 0; i < m_teams.size(); i++)
    {
        if (m_teams[i].active)
        {
            bridgeUsed[m_teams[i].bridge] = true;
        }
        else
        {
            freeColors.push_back((int) i);
        }
    }

    int color  = freeColors[Random().NextInt(0, (int) freeColors.size() - 1)];
    int bridge = 0;
    while (bridgeUsed[bridge])
    {
        bridge++;
    }

    Team &team  = m_teams[color];
    team.active = true;
    team.bridge = bridge;
    team.member = player->GetScoreboardName();
    team.score  = 0;
    BuildBridge(bridge, color, false);

    // The joining player's chunks carry the server's skylight as it is when sent, so the arena has to be lit first
    ServerLevel *level = m_server->GetLevel();
    while (level->GetQueuedLightUpdateCount() > 0)
    {
        level->UpdateLighting(100000);
    }

    Scoreboard &scoreboard = player->GetLevel()->GetScoreboard();
    PlayerTeam *playerTeam = scoreboard.GetTeam(kStickFightColors[color]);
    if (!playerTeam)
    {
        playerTeam = scoreboard.AddTeam(kStickFightColors[color]);
    }

    playerTeam->color        = kStickFightColors[color];
    playerTeam->friendlyFire = false;
    scoreboard.AddMember(team.member, playerTeam);
    GiveKit(player);

    int end = kStickFightCenterRadius + kStickFightBridgeLength - 2;
    BlockPos spawn(kStickFightDirX[bridge] * end, kStickFightArenaY + 6, kStickFightDirZ[bridge] * end);
    player->SetRespawnPosition(spawn, eDimensionId_Overworld, true);
    player->SetPosition(glm::dvec3(spawn.x + 0.5, spawn.y, spawn.z + 0.5));
    player->SetRotation(glm::degrees(atan2f((float) -kStickFightDirZ[bridge], (float) -kStickFightDirX[bridge])), 0.0f);
}

void StickFight::OnPlayerLeft(ServerPlayer *player)
{
    for (size_t i = 0; i < m_teams.size(); i++)
    {
        if (m_teams[i].active && m_teams[i].member == player->GetScoreboardName())
        {
            RememberLayout(player);
            BuildBridge(m_teams[i].bridge, (int) i, true);
            Scoreboard &scoreboard = player->GetLevel()->GetScoreboard();
            scoreboard.RemoveTeam(scoreboard.GetTeam(kStickFightColors[i]));
            m_teams[i].active = false;
        }
    }

    SendState();
}

bool StickFight::OnBlockBroken(ServerPlayer *player, const BlockPos &pos)
{
    for (size_t i = 0; i < m_teams.size(); i++)
    {
        Team &team = m_teams[i];
        if (!team.active || team.member == player->GetScoreboardName())
        {
            continue;
        }

        BlockPos head(team.bed.x + kStickFightDirX[team.bridge], team.bed.y, team.bed.z + kStickFightDirZ[team.bridge]);
        if (pos != team.bed && pos != head)
        {
            continue;
        }

        for (size_t j = 0; j < m_teams.size(); j++)
        {
            if (m_teams[j].active && m_teams[j].member == player->GetScoreboardName())
            {
                m_teams[j].score++;
                SendState();
            }
        }

        ResetRound();
        break;
    }

    return m_arena.count(pos) > 0;
}

void StickFight::OnRespawn(ServerPlayer *player)
{
    for (size_t i = 0; i < m_teams.size(); i++)
    {
        if (m_teams[i].active && m_teams[i].member == player->GetScoreboardName())
        {
            int bridge = m_teams[i].bridge;
            player->SetRotation(glm::degrees(atan2f((float) -kStickFightDirZ[bridge], (float) -kStickFightDirX[bridge])), 0.0f);
        }
    }

    GiveKit(player);
}

void StickFight::ResetRound()
{
    ServerLevel *level = m_server->GetLevel();
    Block *air         = Block::ByName("air");
    int extent         = kStickFightCenterRadius + kStickFightBridgeLength + 8;
    for (int x = -extent; x <= extent; x++)
    {
        for (int z = -extent; z <= extent; z++)
        {
            for (int y = kStickFightArenaY - 10; y <= kStickFightArenaY + 16; y++)
            {
                BlockPos pos(x, y, z);
                // Neighbor updates would pop the other bed half off as an item before we reach it
                if (level->GetBlockId(pos) != 0)
                {
                    level->SetBlockAndDataNoNeighborUpdates(pos, air, 0);
                    level->RecordBlockChange(pos);
                }
            }
        }
    }

    m_arena.clear();
    BuildCenter();
    for (size_t i = 0; i < m_teams.size(); i++)
    {
        if (m_teams[i].active)
        {
            BuildBridge(m_teams[i].bridge, (int) i, false);
        }
    }

    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        ServerPlayer *player = players[i].get();
        if (player->GetHealth() <= 0)
        {
            continue;
        }

        player->SetHealth(player->GetMaxHealth());
        player->SendHealth();
        OnRespawn(player);
        m_server->GetPlayerList()->SendInventoryTo(player);
        const BlockPos &spawn = player->GetRespawnPosition();
        player->GetPlayerConnection()->Teleport(glm::dvec3(spawn.x + 0.5, spawn.y, spawn.z + 0.5), player->GetYaw(), 0.0f);
    }
}

void StickFight::GiveKit(ServerPlayer *player)
{
    RememberLayout(player);
    const std::array<int, 3> &layout = m_layouts.at(player->GetScoreboardName());
    Inventory *inventory             = player->GetInventory();
    for (int i = 0; i < Inventory::s_totalSize; i++)
    {
        inventory->SetSlot(i, ItemInstance());
    }

    inventory->SetCarried(ItemInstance());
    for (int i = 0; i < 3; i++)
    {
        Item *item = Item::ByName(kStickFightKit[i]);
        ItemInstance stack(item, i == 2 ? 64 : 1);
        if (item->IsDamageable())
        {
            CompoundTag *tag = new CompoundTag();
            tag->PutBoolean("Unbreakable", true);
            stack.SetTag(tag);
        }

        inventory->SetSlot(layout[i], stack);
    }
}

void StickFight::Tick()
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i]->GetPosition().y < kStickFightArenaY - 15 && players[i]->GetHealth() > 0)
        {
            players[i]->Hurt(eDamageSource_OutOfWorld, 1000, nullptr);
        }
    }
}

void StickFight::SendState()
{
    std::unique_ptr<CompoundTag> teamsTag(new CompoundTag());
    m_server->GetLevel()->GetScoreboard().Save(teamsTag.get());
    m_server->GetPlayerList()->SendToAll(std::shared_ptr<Packet>(new TeamsS2CPacket(std::move(teamsTag))));

    std::array<int, 4> scores;
    for (size_t i = 0; i < m_teams.size(); i++)
    {
        scores[i] = m_teams[i].active ? m_teams[i].score : -1;
    }

    m_server->GetPlayerList()->SendToAll(std::shared_ptr<Packet>(new StickFightScoresS2CPacket(scores)));
}

void StickFight::BuildCenter()
{
    ServerLevel *level = m_server->GetLevel();
    int extent         = kStickFightCenterRadius + kStickFightBridgeLength + 1;
    for (int x = -extent >> 4; x <= extent >> 4; x++)
    {
        for (int z = -extent >> 4; z <= extent >> 4; z++)
        {
            level->EnsureChunkLoaded(ChunkPos(x, z));
        }
    }

    // The void generator puts a stone plate under the world spawn, which would sit inside the arena
    for (int x = 0; x < 16; x++)
    {
        for (int z = 0; z < 16; z++)
        {
            SetBlock(BlockPos(x, kStickFightArenaY, z), "air");
        }
    }

    for (int x = -kStickFightCenterRadius; x <= kStickFightCenterRadius; x++)
    {
        for (int z = -kStickFightCenterRadius; z <= kStickFightCenterRadius; z++)
        {
            for (int y = kStickFightArenaY - 2; y <= kStickFightArenaY; y++)
            {
                SetBlock(BlockPos(x, y, z), "cut_sandstone");
            }

            SetBlock(BlockPos(x, kStickFightArenaY - 3, z), "bedrock");
        }
    }
}

void StickFight::BuildBridge(int bridge, int color, bool remove)
{
    ServerLevel *level = m_server->GetLevel();
    int dirX           = kStickFightDirX[bridge];
    int dirZ           = kStickFightDirZ[bridge];
    int end            = kStickFightCenterRadius + kStickFightBridgeLength;
    const char *block  = remove ? "air" : "cut_sandstone";
    BlockPos bed(dirX * (end - 3), kStickFightArenaY + 1, dirZ * (end - 3));
    BlockPos head(bed.x + dirX, bed.y, bed.z + dirZ);
    if (remove)
    {
        SetBlock(bed, "air");
        SetBlock(head, "air");
    }

    for (int t = kStickFightCenterRadius + 1; t < end; t++)
    {
        for (int y = kStickFightArenaY - 3; y <= kStickFightArenaY && t < end - 1; y++)
        {
            SetBlock(BlockPos(dirX * t, y, dirZ * t), y == kStickFightArenaY - 3 && !remove ? "bedrock" : block);
        }

        for (int w = -1; w <= 1 && t >= end - 3; w++)
        {
            SetBlock(BlockPos(dirX * t + dirZ * w, kStickFightArenaY + 5, dirZ * t + dirX * w), block);
        }
    }

    if (!remove)
    {
        Block *bedBlock    = Block::ByName(std::string(kStickFightColors[color]) + "_bed");
        m_teams[color].bed = bed;
        level->SetBlock(bed, bedBlock, nullptr, bedBlock->GetPlacementState(level, bed, nullptr, glm::dvec3(0.0), glm::dvec3(dirX, 0.0, dirZ)));
        level->RecordBlockChange(bed);
        m_arena.insert(bed);
        m_arena.insert(head);
    }
}

void StickFight::SetBlock(const BlockPos &pos, const char *name)
{
    ServerLevel *level = m_server->GetLevel();
    level->SetBlock(pos, Block::ByName(name), nullptr);
    level->RecordBlockChange(pos);
    if (name == std::string("air"))
    {
        m_arena.erase(pos);
    }
    else
    {
        m_arena.insert(pos);
    }
}

void StickFight::RememberLayout(ServerPlayer *player)
{
    std::array<int, 3> &layout = m_layouts.try_emplace(player->GetScoreboardName(), std::array<int, 3>{0, 1, 2}).first->second;
    Inventory *inventory       = player->GetInventory();
    bool found[3]              = {};
    for (int i = 0; i < Inventory::s_totalSize; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (!found[j] && inventory->GetSlot(i)->GetItem() == Item::ByName(kStickFightKit[j]))
            {
                layout[j] = i;
                found[j]  = true;
            }
        }
    }

    // A used up kit item keeps its old slot, so we move it if another kit item was sorted there
    for (int j = 0; j < 3; j++)
    {
        int slot = 0;
        while (!found[j] && (layout[j] == layout[(j + 1) % 3] || layout[j] == layout[(j + 2) % 3]))
        {
            layout[j] = slot++;
        }
    }
}
