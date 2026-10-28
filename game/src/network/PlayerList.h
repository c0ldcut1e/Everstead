#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../entity/player/ServerPlayer.h"
#include "../utils/UniqueID.h"
#include "../world/DimensionId.h"
#include "Connection.h"
#include "NetworkTypes.h"
#include "packet/Packet.h"
#include "packet/s2c/DisconnectS2CPacket.h"

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

class EversteadServer;
class EntityTracker;
class Level;

struct BannedPlayer
{
    UniqueID uniqueID;
    std::string name;
};

class PlayerList
{
public:
    explicit PlayerList(EversteadServer *server);
    ~PlayerList();

    ServerPlayer *PlaceNewPlayer(Connection *connection, Pid pid, const std::string &username, const UniqueID &uniqueID);
    void Add(std::unique_ptr<ServerPlayer> player);
    void Remove(uint8_t playerIndex);
    void Clear();
    void DisconnectAll(eDISCONNECT_REASON reason);
    void Tick();
    void SendToAll(std::shared_ptr<Packet> packet);
    void BroadcastChat(const nlohmann::json &component);
    void SendInventoryTo(ServerPlayer *player);
    void SendAttributesTo(ServerPlayer *player);
    void SendDynamicLightsTo(ServerPlayer *player);
    void SendToOthers(ServerPlayer *exceptPlayer, std::shared_ptr<Packet> packet);
    void SendToLevel(Level *level, std::shared_ptr<Packet> packet);

    ServerPlayer *GetPlayer(uint8_t playerIndex);
    ServerPlayer *GetPlayerById(int entityId);
    const std::vector<std::unique_ptr<ServerPlayer>> &GetPlayers() const;
    EntityTracker *GetEntityTracker() const;
    void GetPlayerPositions(Level *level, std::vector<glm::dvec3> *outPtr) const;
    int GetPlayerCount() const;
    bool HasConnection(Connection *connection) const;

    void LoadBans();
    void AddBanned(const UniqueID &uniqueID, const std::string &name);
    bool RemoveBanned(const std::string &name);
    bool IsBanned(const UniqueID &uniqueID) const;
    const std::vector<BannedPlayer> &GetBannedPlayers() const;
    void BroadcastPlayerFlags(ServerPlayer *player);
    void RespawnPlayer(ServerPlayer *player);
    void ChangeDimension(ServerPlayer *player, eDIMENSION_ID dimension, const glm::dvec3 &position, float stability);

private:
    uint8_t FindFreePlayerIndex() const;
    void SendPlayerTo(ServerPlayer *target, ServerPlayer *player);

    EversteadServer *m_server;
    EntityTracker *m_entityTracker;
    std::vector<std::unique_ptr<ServerPlayer>> m_players;
    std::vector<BannedPlayer> m_bannedPlayers;
};
