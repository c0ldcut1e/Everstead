#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../../core/EversteadServer.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../../world/block/Block.h"
#include "../../world/item/Item.h"
#include "../PlayerList.h"
#include "../packet/Packet.h"
#include "../packet/s2c/MultiBlockUpdateS2CPacket.h"

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

class CommandContext
{
public:
    CommandContext(EversteadServer *server, ServerPlayer *source);
    ~CommandContext();

    EversteadServer *GetServer() const;
    PlayerList *GetPlayerList() const;
    ServerPlayer *GetCaller() const;
    Entity *GetEntity() const;
    void SetEntity(Entity *entity);
    ServerPlayer *GetPlayer() const;
    bool RequirePlayer(ServerPlayer **player) const;
    const glm::dvec3 &GetPosition() const;
    void SetPosition(const glm::dvec3 &position);
    glm::dvec3 GetAnchorPosition() const;
    void SetAnchoredAtEyes(bool anchoredAtEyes);
    float GetYaw() const;
    float GetPitch() const;
    void SetRotation(float yaw, float pitch);
    void FacePosition(const glm::dvec3 &target);
    glm::dvec3 GetFront() const;
    void SetAdminCommand(bool adminCommand);
    ServerLevel *GetLevel() const;
    void SetLevel(ServerLevel *level);
    bool HasFailed() const;
    Block *FindBlock(const std::string &name) const;
    Item *FindItem(const std::string &name) const;
    void SetBlock(const BlockPos &pos, Block *block, uint16_t data);
    void FlushBlockChanges();

    static nlohmann::json Translate(const std::string &key, const std::vector<std::wstring> &arguments = {});
    void SendSystemMessage(const nlohmann::json &message, bool error) const;
    void SendChatMessage(ServerPlayer *target, const std::string &message, bool advanced) const;
    void SendPacket(ServerPlayer *target, std::shared_ptr<Packet> packet) const;
    void TeleportPlayer(ServerPlayer *target, const glm::dvec3 &position, float yaw, float pitch) const;
    void BanPlayer(ServerPlayer *target) const;
    std::wstring GetPlayerDisplayName(ServerPlayer *player) const;
    std::wstring JoinPlayerNames(const std::vector<ServerPlayer *> &players) const;

private:
    EversteadServer *m_server;
    ServerPlayer *m_source;
    Entity *m_entity;
    glm::dvec3 m_position;
    float m_yaw;
    float m_pitch;
    ServerLevel *m_level;
    bool m_anchoredAtEyes;
    bool m_adminCommand;
    mutable bool m_failed;
    std::vector<MultiBlockUpdateS2CPacket::Change> m_blockChanges;
};
