#pragma once

#include <chrono>
#include <cstddef>
#include <memory>

#include "../../core/EversteadServer.h"
#include "../../entity/player/ServerPlayer.h"
#include "../Connection.h"
#include "../PlayerChunkSender.h"
#include "../PlayerList.h"
#include "../command/CommandDispatcher.h"
#include "../packet/Packet.h"
#include "PacketListener.h"

#include <glm/vec3.hpp>

class PlayerConnection : public PacketListener
{
public:
    PlayerConnection(EversteadServer *server, Connection *connection, ServerPlayer *player);
    ~PlayerConnection() override;

    void Tick();
    void Send(std::shared_ptr<Packet> packet);
    void Disconnect(eDISCONNECT_REASON reason, const std::wstring &text = L"");

    bool IsServerPacketListener() override;
    void OnUnhandledPacket(Packet *packet) override;
    void HandleDisconnect(DisconnectC2SPacket *packet) override;
    void HandleMovePlayer(MovePlayerC2SPacket *packet) override;
    void HandlePlayerAction(PlayerActionC2SPacket *packet) override;
    void HandleChat(ChatC2SPacket *packet) override;
    void HandleSwing(SwingC2SPacket *packet) override;
    void HandleDynamicLight(DynamicLightC2SPacket *packet) override;
    void HandleFogVolume(FogVolumeC2SPacket *packet) override;
    void HandleAtmosphere(AtmosphereC2SPacket *packet) override;
    void HandleRespawn(RespawnC2SPacket *packet) override;
    void HandleChunkAck(ChunkAckC2SPacket *packet) override;
    void HandleAttack(AttackC2SPacket *packet) override;
    void HandleInteract(InteractC2SPacket *packet) override;
    void HandleMoveVehicle(MoveVehicleC2SPacket *packet) override;
    void HandlePlayerInput(PlayerInputC2SPacket *packet) override;
    void HandleRenameItem(RenameItemC2SPacket *packet) override;
    void HandleTabComplete(TabCompleteC2SPacket *packet) override;
    void HandleSetCarriedItem(SetCarriedItemC2SPacket *packet) override;
    void HandleContainerClick(ContainerClickC2SPacket *packet) override;
    void HandlePickBlock(PickBlockC2SPacket *packet) override;
    void HandleSignUpdate(SignUpdateC2SPacket *packet) override;

    Connection *GetConnection() const;
    ServerPlayer *GetPlayer() const;
    PlayerChunkSender *GetChunkSender() const;
    void Teleport(const glm::dvec3 &position, float yaw, float pitch);

private:
    static inline const int s_timeoutSeconds  = 30;
    static const int s_keepAliveIntervalTicks = 20;
    static const size_t s_maxChatLength       = 255;
    static const double s_maxMoveDistance;
    static const double s_maxSpectatorSpeedScale;
    static const double s_minWorldY;
    static const double s_maxWorldY;

    void ResendPosition();

    EversteadServer *m_server;
    Connection *m_connection;
    ServerPlayer *m_player;
    CommandDispatcher *m_commandDispatcher;
    PlayerChunkSender *m_chunkSender;
    std::chrono::steady_clock::time_point m_lastPacketTime;
    int m_ticksSinceKeepAlive;
    uint32_t m_teleportId;
};
