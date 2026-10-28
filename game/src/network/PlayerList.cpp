#include "PlayerList.h"

#include <algorithm>
#include <cmath>

#include <nlohmann/json.hpp>

#include "../core/EversteadServer.h"
#include "../core/GameInfo.h"
#include "../core/Logger.h"
#include "../entity/EntityTracker.h"
#include "../entity/player/GameType.h"
#include "../entity/player/ServerPlayer.h"
#include "../tag/CompoundTag.h"
#include "../ui/ChatMessageParser.h"
#include "../utils/StringUtils.h"
#include "../world/Level.h"
#include "../world/ServerLevel.h"
#include "../world/chunk/ChunkManager.h"
#include "../world/inventory/InventoryMenu.h"
#include "../world/inventory/ServerContainerMenu.h"
#include "../world/save/WorldSave.h"
#include "Connection.h"
#include "listener/PlayerConnection.h"
#include "packet/Packet.h"
#include "packet/s2c/AddPlayerS2CPacket.h"
#include "packet/s2c/AtmosphereS2CPacket.h"
#include "packet/s2c/ChangeDimensionS2CPacket.h"
#include "packet/s2c/ChatS2CPacket.h"
#include "packet/s2c/ContainerSetContentS2CPacket.h"
#include "packet/s2c/DisconnectS2CPacket.h"
#include "packet/s2c/DynamicLightS2CPacket.h"
#include "packet/s2c/FogVolumeS2CPacket.h"
#include "packet/s2c/InfoS2CPacket.h"
#include "packet/s2c/LevelReadyS2CPacket.h"
#include "packet/s2c/LoginS2CPacket.h"
#include "packet/s2c/RemovePlayerS2CPacket.h"
#include "packet/s2c/SetEntityDataS2CPacket.h"
#include "packet/s2c/SetEquipmentS2CPacket.h"
#include "packet/s2c/SetGameModeS2CPacket.h"
#include "packet/s2c/SetHealthS2CPacket.h"
#include "packet/s2c/SetPlayerPositionS2CPacket.h"
#include "packet/s2c/SetTimeS2CPacket.h"
#include "packet/s2c/SetWeatherS2CPacket.h"
#include "packet/s2c/TeamsS2CPacket.h"
#include "packet/s2c/TickingStateS2CPacket.h"
#include "packet/s2c/UpdateAttributesS2CPacket.h"
#include "packet/s2c/WorldBorderS2CPacket.h"

PlayerList::PlayerList(EversteadServer *server)
{
    m_server        = server;
    m_entityTracker = new EntityTracker(this);
    m_bannedPlayers = {};
}

PlayerList::~PlayerList()
{
    Clear();
    delete m_entityTracker;
}

ServerPlayer *PlayerList::PlaceNewPlayer(Connection *connection, Pid pid, const std::string &username, const UniqueID &uniqueID)
{
    std::wstring wideUsername(username.begin(), username.end());
    ServerLevel *worldLevel = m_server->GetLevel(eDimensionId_Overworld);
    if (IsBanned(uniqueID))
    {
        connection->QueueIncomingPacket(std::shared_ptr<Packet>(new DisconnectS2CPacket(eDisconnectReason_Banned)));
        connection->CloseAfterFlush(eDisconnectReason_Banned);
        Logger::LogInfo("Player list rejected banned player pid=%u username=%s", pid, username.c_str());
        return nullptr;
    }

    for (size_t i = 0; i < m_players.size(); i++)
    {
        if (m_players[i]->GetUniqueID() == uniqueID)
        {
            Logger::LogInfo("Player list removed old player before rejoin pid=%u playerIndex=%u", pid, m_players[i]->GetPlayerIndex());
            Remove(m_players[i]->GetPlayerIndex());
            break;
        }
    }

    if (!m_server->settings.IsWhitelisted(username))
    {
        connection->QueueIncomingPacket(std::shared_ptr<Packet>(new DisconnectS2CPacket(eDisconnectReason_NotWhitelisted)));
        connection->CloseAfterFlush(eDisconnectReason_NotWhitelisted);
        Logger::LogInfo("Player list rejected player pid=%u username=%s reason=not whitelisted", pid, username.c_str());
        return nullptr;
    }

    uint8_t playerIndex = FindFreePlayerIndex();
    if (GetPlayerCount() >= m_server->GetMaxPlayers())
    {
        connection->QueueIncomingPacket(std::shared_ptr<Packet>(new DisconnectS2CPacket(eDisconnectReason_MaxPlayers)));
        connection->CloseAfterFlush(eDisconnectReason_MaxPlayers);
        Logger::LogInfo("Player list rejected player pid=%u username=%s reason=max players", pid, username.c_str());
        return nullptr;
    }

    std::unique_ptr<ServerPlayer> player(new ServerPlayer(m_server, worldLevel, wideUsername, playerIndex));
    ServerPlayer *playerPtr = player.get();
    playerPtr->SetUniqueID(uniqueID);
    playerPtr->SetId(m_server->NextEntityId());
    playerPtr->SetPid(pid);
    playerPtr->SetOperator(m_server->settings.IsOperator(pid) || wideUsername == L"Server");
    playerPtr->SetPlayerConnection(new PlayerConnection(m_server, connection, playerPtr));
    Add(std::move(player));

    Logger::LogInfo("Player list placed player pid=%u username=%s playerIndex=%u count=%d", pid, username.c_str(), playerIndex, GetPlayerCount());

    std::string worldName   = m_server->GetWorldName().empty() ? "New World" : m_server->GetWorldName();
    eDIMENSION_ID dimension = eDimensionId_Overworld;
    playerPtr->SetGameType(worldLevel->GetDefaultGameType());
    std::vector<ItemInstance> backpackItems;
    bool loadedPlayerData = WorldSave::LoadPlayerData(worldName, playerPtr, &dimension, &backpackItems);
    if (dimension != eDimensionId_Overworld)
    {
        worldLevel = m_server->GetOrCreateLevel(dimension);
        playerPtr->SetLevel(worldLevel);
    }

    glm::dvec3 loadedPosition = playerPtr->GetPosition();
    if (!loadedPlayerData || !std::isfinite(loadedPosition.x) || !std::isfinite(loadedPosition.y) || !std::isfinite(loadedPosition.z))
    {
        playerPtr->SetPosition(worldLevel->FindPlayerSpawn(worldLevel->GetWorldSpawn()));
    }

    if (kStickFight)
    {
        m_server->GetStickFight()->OnPlayerJoined(playerPtr);
    }

    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new LoginS2CPacket()));
    playerPtr->GetPlayerConnection()->Send(
            std::shared_ptr<Packet>(new InfoS2CPacket(playerIndex, playerPtr->GetId(), (uint8_t) m_server->GetMaxPlayers(),
                                                      (uint8_t) GetPlayerCount(), m_server->GetWorldName(), m_server->settings.serverName)));
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ChangeDimensionS2CPacket(dimension, 1.0f)));
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(
            new SetTimeS2CPacket(worldLevel->GetWorldProvider().GetWorldTime(), worldLevel->GetGameRules().GetBool(eGameRule_DoDaylightCycle))));
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(
            new TickingStateS2CPacket(m_server->GetTickRateManager()->GetTickRate(), m_server->GetTickRateManager()->IsFrozen())));
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new SetWeatherS2CPacket(
            worldLevel->IsRaining(), worldLevel->IsThundering(), worldLevel->GetRainLevel(1.0f), worldLevel->GetThunderLevel(1.0f))));
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new WorldBorderS2CPacket(worldLevel->GetWorldBorder())));
    std::unique_ptr<CompoundTag> teamsTag(new CompoundTag());
    worldLevel->GetScoreboard().Save(teamsTag.get());
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new TeamsS2CPacket(std::move(teamsTag))));
    SendDynamicLightsTo(playerPtr);

    glm::dvec3 spawnPos = playerPtr->GetPosition();
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new SetPlayerPositionS2CPacket(
            spawnPos.x, spawnPos.y, spawnPos.z, playerPtr->GetYaw(), playerPtr->GetPitch(), 0, playerPtr->GetVelocity())));
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new SetEntityDataS2CPacket(playerIndex, playerPtr->GetSharedFlags())));
    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new SetGameModeS2CPacket(playerPtr->GetGameType(), playerPtr->IsFlying())));
    playerPtr->SendHealth();
    playerPtr->SendExperience();
    SendInventoryTo(playerPtr);
    SendAttributesTo(playerPtr);
    worldLevel->GetChunkManager()->Update(spawnPos);
    for (size_t i = 0; i < backpackItems.size(); i++)
    {
        playerPtr->DropItem(backpackItems[i]);
    }

    if (!backpackItems.empty())
    {
        Logger::LogInfo("Player list dropped %zu backpack stacks for playerIndex=%u", backpackItems.size(), playerIndex);
    }

    playerPtr->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new LevelReadyS2CPacket()));
    m_entityTracker->Track(playerPtr);
    worldLevel->SendEntitiesTo(playerPtr);

    for (std::unique_ptr<ServerPlayer> &otherPlayer : m_players)
    {
        if (otherPlayer.get() != playerPtr && otherPlayer->GetLevel() == worldLevel)
        {
            SendPlayerTo(playerPtr, otherPlayer.get());
            SendPlayerTo(otherPlayer.get(), playerPtr);
        }
    }

    nlohmann::json component;
    component["translate"] = "multiplayer.player.joined";
    component["color"]     = "yellow";
    component["with"]      = nlohmann::json::array();
    component["with"].push_back(playerPtr->GetDisplayName());
    if (GetPlayerCount() > 1)
    {
        BroadcastChat(component);
    }
    else
    {
        Logger::LogInfo("[Chat] %s", ChatMessageParser().ToPlainText(component).c_str());
    }

    if (kStickFight)
    {
        m_server->GetStickFight()->SendState();
    }

    Logger::LogInfo("Player list placed spawn=%f,%f,%f playerIndex=%u", spawnPos.x, spawnPos.y, spawnPos.z, playerIndex);
    return playerPtr;
}

void PlayerList::Add(std::unique_ptr<ServerPlayer> player)
{
    m_players.push_back(std::move(player));
}

void PlayerList::Remove(uint8_t playerIndex)
{
    size_t i = 0;
    while (i < m_players.size())
    {
        ServerPlayer *player = m_players[i].get();
        if (player->GetPlayerIndex() == playerIndex)
        {
            m_entityTracker->Untrack(player);
            player->GetContainerMenu()->CloseAll();
            player->WakeUp();

            std::string worldName = m_server->GetWorldName().empty() ? "New World" : m_server->GetWorldName();
            WorldSave::SavePlayerData(worldName, player);
            nlohmann::json component;
            component["translate"] = "multiplayer.player.left";
            component["color"]     = "yellow";
            component["with"]      = nlohmann::json::array();
            component["with"].push_back(player->GetDisplayName());
            if (GetPlayerCount() > 1 && !IsBanned(player->GetUniqueID()))
            {
                BroadcastChat(component);
            }
            else
            {
                Logger::LogInfo("[Chat] %s", ChatMessageParser().ToPlainText(component).c_str());
            }

            SendToOthers(player, std::shared_ptr<Packet>(new RemovePlayerS2CPacket(player->GetPlayerIndex())));
            if (kStickFight)
            {
                m_server->GetStickFight()->OnPlayerLeft(player);
            }

            m_players.erase(m_players.begin() + (long) i);
        }
        else
        {
            i++;
        }
    }
}

void PlayerList::Clear()
{
    m_entityTracker->Clear();
    m_players.clear();
}

void PlayerList::DisconnectAll(eDISCONNECT_REASON reason)
{
    for (std::unique_ptr<ServerPlayer> &player : m_players)
    {
        player->GetPlayerConnection()->Disconnect(reason);
    }
}

void PlayerList::Tick()
{
    for (std::unique_ptr<ServerPlayer> &player : m_players)
    {
        player->Tick();
    }

    const std::vector<ServerLevel *> &levels = m_server->GetLevels();
    for (size_t levelIndex = 0; levelIndex < levels.size(); levelIndex++)
    {
        ServerLevel *level = levels[levelIndex];
        int activePlayers  = 0;
        int deepSleepers   = 0;
        for (size_t i = 0; i < m_players.size(); i++)
        {
            if (m_players[i]->GetLevel() != level || GameType::IsSpectator(m_players[i]->GetGameType()))
            {
                continue;
            }

            activePlayers++;
            if (m_players[i]->IsSleeping() && m_players[i]->GetSleepTimer() >= 100)
            {
                deepSleepers++;
            }
        }

        int sleepersNeeded = std::max(1, (activePlayers * level->GetGameRules().GetInt(eGameRule_PlayersSleepingPercentage) + 99) / 100);
        if (deepSleepers > 0 && deepSleepers >= sleepersNeeded)
        {
            int64_t ticks = level->GetWorldProvider().GetWorldTime();
            ticks         = ticks - ticks % WorldProvider::s_dayTicks + WorldProvider::s_dayTicks;
            level->SetWorldTime(ticks);
            SendToLevel(level, std::shared_ptr<Packet>(new SetTimeS2CPacket(ticks, level->GetGameRules().GetBool(eGameRule_DoDaylightCycle))));
            level->SetWeather(false, 0, false, 0);
            for (size_t i = 0; i < m_players.size(); i++)
            {
                if (m_players[i]->GetLevel() == level)
                {
                    m_players[i]->WakeUp();
                }
            }
        }
    }

    size_t i = 0;
    while (i < m_players.size())
    {
        ServerPlayer *player = m_players[i].get();
        if (player->GetPlayerConnection()->GetConnection()->IsClosed())
        {
            Logger::LogInfo("Player list removed disconnected player playerIndex=%u", player->GetPlayerIndex());
            Remove(player->GetPlayerIndex());
        }
        else
        {
            i++;
        }
    }

    m_entityTracker->Tick();
}

ServerPlayer *PlayerList::GetPlayer(uint8_t playerIndex)
{
    for (std::unique_ptr<ServerPlayer> &player : m_players)
    {
        if (player->GetPlayerIndex() == playerIndex)
        {
            return player.get();
        }
    }

    return nullptr;
}

ServerPlayer *PlayerList::GetPlayerById(int entityId)
{
    for (size_t i = 0; i < m_players.size(); i++)
    {
        if (m_players[i]->GetId() == entityId)
        {
            return m_players[i].get();
        }
    }

    return nullptr;
}

const std::vector<std::unique_ptr<ServerPlayer>> &PlayerList::GetPlayers() const
{
    return m_players;
}

EntityTracker *PlayerList::GetEntityTracker() const
{
    return m_entityTracker;
}

void PlayerList::GetPlayerPositions(Level *level, std::vector<glm::dvec3> *outPtr) const
{
    outPtr->clear();
    for (size_t i = 0; i < m_players.size(); i++)
    {
        if (m_players[i]->GetLevel() == level)
        {
            outPtr->push_back(m_players[i]->GetPosition());
        }
    }
}

int PlayerList::GetPlayerCount() const
{
    return (int) m_players.size();
}

bool PlayerList::HasConnection(Connection *connection) const
{
    for (size_t i = 0; i < m_players.size(); i++)
    {
        if (m_players[i]->GetPlayerConnection() && m_players[i]->GetPlayerConnection()->GetConnection() == connection)
        {
            return true;
        }
    }

    return false;
}

void PlayerList::LoadBans()
{
    m_bannedPlayers = WorldSave::ReadBans(m_server->GetWorldName().empty() ? "New World" : m_server->GetWorldName());
}

void PlayerList::AddBanned(const UniqueID &uniqueID, const std::string &name)
{
    if (IsBanned(uniqueID))
    {
        return;
    }

    m_bannedPlayers.push_back({uniqueID, name});
    WorldSave::WriteBans(m_server->GetWorldName().empty() ? "New World" : m_server->GetWorldName(), m_bannedPlayers);
}

bool PlayerList::RemoveBanned(const std::string &name)
{
    std::string lowerName = StringUtils::ToLower(name);
    for (size_t i = 0; i < m_bannedPlayers.size(); i++)
    {
        if (StringUtils::ToLower(m_bannedPlayers[i].name) == lowerName)
        {
            m_bannedPlayers.erase(m_bannedPlayers.begin() + (ptrdiff_t) i);
            WorldSave::WriteBans(m_server->GetWorldName().empty() ? "New World" : m_server->GetWorldName(), m_bannedPlayers);
            return true;
        }
    }

    return false;
}

bool PlayerList::IsBanned(const UniqueID &uniqueID) const
{
    for (size_t i = 0; i < m_bannedPlayers.size(); i++)
    {
        if (m_bannedPlayers[i].uniqueID == uniqueID)
        {
            return true;
        }
    }

    return false;
}

const std::vector<BannedPlayer> &PlayerList::GetBannedPlayers() const
{
    return m_bannedPlayers;
}

void PlayerList::BroadcastPlayerFlags(ServerPlayer *player)
{
    SendToAll(std::shared_ptr<Packet>(new SetEntityDataS2CPacket(player->GetPlayerIndex(), player->GetSharedFlags())));
}

uint8_t PlayerList::FindFreePlayerIndex() const
{
    // Clients show index 0 as the host, so a dedicated server never hands it out
    for (uint16_t playerIndex = m_server->settings.dedicated ? 1 : 0; playerIndex < 256; playerIndex++)
    {
        bool used = false;
        for (size_t i = 0; i < m_players.size(); i++)
        {
            if (m_players[i]->GetPlayerIndex() == playerIndex)
            {
                used = true;
                break;
            }
        }

        if (!used)
        {
            return (uint8_t) playerIndex;
        }
    }

    return 255;
}

void PlayerList::RespawnPlayer(ServerPlayer *player)
{
    SendToOthers(player, std::shared_ptr<Packet>(new RemovePlayerS2CPacket(player->GetPlayerIndex())));
    for (std::unique_ptr<ServerPlayer> &target : m_players)
    {
        if (target.get() != player && target->GetLevel() == player->GetLevel())
        {
            SendPlayerTo(target.get(), player);
        }
    }
}

void PlayerList::ChangeDimension(ServerPlayer *player, eDIMENSION_ID dimension, const glm::dvec3 &position, float stability)
{
    ServerLevel *level = m_server->GetOrCreateLevel(dimension);
    player->GetContainerMenu()->CloseAll();
    player->WakeUp();
    player->StopRiding();
    SendToOthers(player, std::shared_ptr<Packet>(new RemovePlayerS2CPacket(player->GetPlayerIndex())));
    m_entityTracker->Untrack(player);

    player->SetLevel(level);
    PlayerConnection *connection = player->GetPlayerConnection();
    connection->GetChunkSender()->Reset();
    connection->Send(std::shared_ptr<Packet>(new ChangeDimensionS2CPacket(dimension, stability)));
    connection->Teleport(position, player->GetYaw(), 0.0f);
    connection->Send(std::shared_ptr<Packet>(
            new SetTimeS2CPacket(level->GetWorldProvider().GetWorldTime(), level->GetGameRules().GetBool(eGameRule_DoDaylightCycle))));
    connection->Send(std::shared_ptr<Packet>(
            new SetWeatherS2CPacket(level->IsRaining(), level->IsThundering(), level->GetRainLevel(1.0f), level->GetThunderLevel(1.0f))));
    connection->Send(std::shared_ptr<Packet>(new WorldBorderS2CPacket(level->GetWorldBorder())));
    SendDynamicLightsTo(player);
    level->GetChunkManager()->Update(player->GetPosition());

    m_entityTracker->Track(player);
    level->SendEntitiesTo(player);
    for (std::unique_ptr<ServerPlayer> &otherPlayer : m_players)
    {
        if (otherPlayer.get() != player && otherPlayer->GetLevel() == level)
        {
            SendPlayerTo(player, otherPlayer.get());
            SendPlayerTo(otherPlayer.get(), player);
        }
    }

    Logger::LogInfo("Player list moved playerIndex=%u to dimension %s", player->GetPlayerIndex(), kDimensionNames[dimension]);
}

void PlayerList::SendPlayerTo(ServerPlayer *target, ServerPlayer *player)
{
    if (player->GetHealth() <= 0)
    {
        return;
    }

    glm::dvec3 position       = player->GetPosition();
    std::wstring wideUsername = player->GetName();
    std::string username(wideUsername.begin(), wideUsername.end());
    target->GetPlayerConnection()->Send(
            std::shared_ptr<Packet>(new AddPlayerS2CPacket(player->GetPlayerIndex(), player->GetId(), username, position.x, position.y, position.z,
                                                           player->GetYaw(), player->GetPitch(), player->GetSharedFlags())));

    ItemInstance *heldItem = player->GetInventory()->GetSelectedItem();
    target->GetPlayerConnection()->Send(
            std::shared_ptr<Packet>(new SetEquipmentS2CPacket(player->GetPlayerIndex(), 0, heldItem ? *heldItem : ItemInstance())));
    for (int i = 0; i < Inventory::s_armorSize; i++)
    {
        target->GetPlayerConnection()->Send(std::shared_ptr<Packet>(
                new SetEquipmentS2CPacket(player->GetPlayerIndex(), (uint8_t) (i + 1), *player->GetInventory()->GetArmor(i))));
    }
}

void PlayerList::SendInventoryTo(ServerPlayer *player)
{
    Inventory *inventory = player->GetInventory();

    std::vector<ItemInstance> items;
    for (int slot = 0; slot < Inventory::s_slotCount; slot++)
    {
        ItemInstance *item = inventory->GetSlot(slot);
        items.push_back(item ? *item : ItemInstance());
    }

    CraftingContainer *crafting = player->GetInventoryCraftingContainer();
    for (int slot = 0; slot < crafting->GetSize(); slot++)
    {
        items.push_back(*crafting->GetItem(slot));
    }

    player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(
            new ContainerSetContentS2CPacket(InventoryMenu::s_playerMenuId, (uint8_t) inventory->GetSelectedSlot(), items, inventory->GetCarried())));
}

void PlayerList::SendAttributesTo(ServerPlayer *player)
{
    player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new UpdateAttributesS2CPacket(*player->GetAttributes())));
}

void PlayerList::SendDynamicLightsTo(ServerPlayer *player)
{
    PlayerConnection *connection = player->GetPlayerConnection();
    DynamicLights &dynamicLights = player->GetLevel()->GetDynamicLights();
    for (size_t i = 0; i < dynamicLights.GetLights().size(); i++)
    {
        connection->Send(std::shared_ptr<Packet>(new DynamicLightS2CPacket(dynamicLights.GetLights()[i])));
    }

    for (size_t i = 0; i < dynamicLights.GetFogVolumes().size(); i++)
    {
        connection->Send(std::shared_ptr<Packet>(new FogVolumeS2CPacket(dynamicLights.GetFogVolumes()[i])));
    }

    connection->Send(std::shared_ptr<Packet>(new AtmosphereS2CPacket(dynamicLights.GetAirDensity(), dynamicLights.GetAnisotropy())));
}

void PlayerList::SendToAll(std::shared_ptr<Packet> packet)
{
    for (std::unique_ptr<ServerPlayer> &target : m_players)
    {
        target->GetPlayerConnection()->Send(packet);
    }
}

void PlayerList::BroadcastChat(const nlohmann::json &component)
{
    Logger::LogInfo("[Chat] %s", ChatMessageParser().ToPlainText(component).c_str());
    SendToAll(std::shared_ptr<Packet>(new ChatS2CPacket(component.dump(), true)));
}

void PlayerList::SendToOthers(ServerPlayer *exceptPlayer, std::shared_ptr<Packet> packet)
{
    for (std::unique_ptr<ServerPlayer> &target : m_players)
    {
        if (target.get() != exceptPlayer && target->GetLevel() == exceptPlayer->GetLevel())
        {
            target->GetPlayerConnection()->Send(packet);
        }
    }
}

void PlayerList::SendToLevel(Level *level, std::shared_ptr<Packet> packet)
{
    for (std::unique_ptr<ServerPlayer> &target : m_players)
    {
        if (target->GetLevel() == level)
        {
            target->GetPlayerConnection()->Send(packet);
        }
    }
}
