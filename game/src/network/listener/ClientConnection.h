#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../entity/player/RemotePlayer.h"
#include "../../utils/UniqueID.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/chunk/ChunkPos.h"
#include "../Connection.h"
#include "../NetworkTypes.h"
#include "../packet/s2c/ServerStatusS2CPacket.h"
#include "PacketListener.h"

struct ClientPlayerListEntry
{
    std::wstring name;
    uint8_t playerIndex;
    bool isOperator;
    int ping;
};

class ClientConnection : public PacketListener
{
public:
    explicit ClientConnection(Connection *connection);
    ~ClientConnection() override;

    void Tick();
    void ResetTimeout();
    void BeginLogin(Pid pid, const std::string &username, const UniqueID &uniqueID);
    void Send(std::shared_ptr<Packet> packet);

    bool IsServerPacketListener() override;
    void HandleDisconnect(DisconnectS2CPacket *packet) override;
    void HandleLogin(LoginS2CPacket *packet) override;
    void HandleInfo(InfoS2CPacket *packet) override;
    void HandleLevelReady(LevelReadyS2CPacket *packet) override;
    void HandleChunkBlocksUpdate(ChunkBlocksUpdateS2CPacket *packet) override;
    void HandleSetPlayerPosition(SetPlayerPositionS2CPacket *packet) override;
    void HandleAddPlayer(AddPlayerS2CPacket *packet) override;
    void HandleMovePlayer(MovePlayerS2CPacket *packet) override;
    void HandleMoveEntityPos(MoveEntityPosS2CPacket *packet) override;
    void HandleMoveEntityRot(MoveEntityRotS2CPacket *packet) override;
    void HandleMoveEntityPosRot(MoveEntityPosRotS2CPacket *packet) override;
    void HandleRotateHead(RotateHeadS2CPacket *packet) override;
    void HandleSetEntityMotion(SetEntityMotionS2CPacket *packet) override;
    void HandleTeleportEntity(TeleportEntityS2CPacket *packet) override;
    void HandleSound(SoundS2CPacket *packet) override;
    void HandleStopSound(StopSoundS2CPacket *packet) override;
    void HandleParticle(ParticleS2CPacket *packet) override;
    void HandleSetGameMode(SetGameModeS2CPacket *packet) override;
    void HandleSetHealth(SetHealthS2CPacket *packet) override;
    void HandleSetExperience(SetExperienceS2CPacket *packet) override;
    void HandleRemovePlayer(RemovePlayerS2CPacket *packet) override;
    void HandleBlockUpdate(BlockUpdateS2CPacket *packet) override;
    void HandleChat(ChatS2CPacket *packet) override;
    void HandlePlayerDeath(PlayerDeathS2CPacket *packet) override;
    void HandleSwing(SwingS2CPacket *packet) override;
    void HandleBlockDestruction(BlockDestructionS2CPacket *packet) override;
    void HandleAnimateHurt(AnimateHurtS2CPacket *packet) override;
    void HandleAnimateCrit(AnimateCritS2CPacket *packet) override;
    void HandleKnockback(KnockbackS2CPacket *packet) override;
    void HandleExplode(ExplodeS2CPacket *packet) override;
    void HandleTabComplete(TabCompleteS2CPacket *packet) override;
    void HandleMultiBlockUpdate(MultiBlockUpdateS2CPacket *packet) override;
    void HandleUpdateAttributes(UpdateAttributesS2CPacket *packet) override;
    void HandleServerStatus(ServerStatusS2CPacket *packet) override;
    void HandleChangeDimension(ChangeDimensionS2CPacket *packet) override;
    void HandleSetTime(SetTimeS2CPacket *packet) override;
    void HandleTickingState(TickingStateS2CPacket *packet) override;
    void HandleTickingStep(TickingStepS2CPacket *packet) override;
    void HandleStickFightScores(StickFightScoresS2CPacket *packet) override;
    void HandleSetWeather(SetWeatherS2CPacket *packet) override;
    void HandleWorldBorder(WorldBorderS2CPacket *packet) override;
    void HandleDynamicLight(DynamicLightS2CPacket *packet) override;
    void HandleFogVolume(FogVolumeS2CPacket *packet) override;
    void HandleAtmosphere(AtmosphereS2CPacket *packet) override;
    void HandleDynamicLightMenu(DynamicLightMenuS2CPacket *packet) override;
    void HandleAddEntity(AddEntityS2CPacket *packet) override;
    void HandleTakeItemEntity(TakeItemEntityS2CPacket *packet) override;
    void HandleRemoveEntities(RemoveEntitiesS2CPacket *packet) override;
    void HandleSetEntityData(SetEntityDataS2CPacket *packet) override;
    void HandleSetMobData(SetMobDataS2CPacket *packet) override;
    void HandleEntityEvent(EntityEventS2CPacket *packet) override;
    void HandleSetPassengers(SetPassengersS2CPacket *packet) override;
    void HandleSetBoatData(SetBoatDataS2CPacket *packet) override;
    void HandleContainerOpen(ContainerOpenS2CPacket *packet) override;
    void HandleChestOpen(ChestOpenS2CPacket *packet) override;
    void HandlePlayerSleep(PlayerSleepS2CPacket *packet) override;
    void HandleSpawnerData(SpawnerDataS2CPacket *packet) override;
    void HandleSignData(SignDataS2CPacket *packet) override;
    void HandleMovingPiston(MovingPistonS2CPacket *packet) override;
    void HandleTeams(TeamsS2CPacket *packet) override;
    void HandleWorldEffect(WorldEffectS2CPacket *packet) override;
    void HandleOpenSignEditor(OpenSignEditorS2CPacket *packet) override;
    void HandleContainerClose(ContainerCloseS2CPacket *packet) override;
    void HandleContainerSetContent(ContainerSetContentS2CPacket *packet) override;
    void HandleContainerSetData(ContainerSetDataS2CPacket *packet) override;
    void HandleContainerSetSlot(ContainerSetSlotS2CPacket *packet) override;
    void HandleSetEquipment(SetEquipmentS2CPacket *packet) override;

    Connection *GetConnection() const;
    bool IsLoggedIn() const;
    bool CanEnterWorld() const;
    uint32_t GetTeleportId() const;
    bool IsDisconnected() const;
    void ForceDisconnect(eDISCONNECT_REASON reason);
    eDISCONNECT_REASON GetDisconnectReason() const;
    const std::wstring &GetDisconnectMessage() const;
    uint8_t GetPlayerIndex() const;
    RemotePlayer *FindRemotePlayer(uint8_t playerIndex);
    std::vector<ClientPlayerListEntry> GetPlayerList() const;
    const std::string &GetWorldName() const;
    const std::string &GetServerName() const;
    int GetMaxPlayers() const;
    float GetTps() const;
    float GetMspt() const;

private:
    static inline const int s_timeoutSeconds  = 30;
    static const int s_keepAliveIntervalTicks = 20;

    Entity *FindMovedEntity(int entityId);
    RemotePlayer *AddRemotePlayer(uint8_t playerIndex, const std::string &username);

    Connection *m_connection;
    Pid m_pid;
    std::string m_username;
    UniqueID m_uniqueID;
    std::map<uint8_t, RemotePlayer *> m_remotePlayers;
    bool m_loggedIn;
    bool m_receivedInfo;
    bool m_receivedPlayerPosition;
    uint32_t m_teleportId;
    int m_unackedChunks;
    int m_ackedRenderDistance;
    bool m_levelReady;
    bool m_changingDimension;
    std::unordered_set<ChunkPos, ChunkPosHash> m_staleChunks;
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> m_parkedChunks;
    std::vector<std::shared_ptr<Packet>> m_heldPackets;
    bool m_disconnected;
    eDISCONNECT_REASON m_disconnectReason;
    std::wstring m_disconnectMessage;
    uint8_t m_playerIndex;
    int m_initialRemotePlayersRemaining;
    std::chrono::steady_clock::time_point m_lastPacketTime;
    int m_ticksSinceKeepAlive;
    std::string m_worldName;
    std::string m_serverName;
    int m_maxPlayers;
    float m_tps;
    float m_mspt;
    std::vector<ServerStatusS2CPacket::PlayerInfo> m_statusPlayers;
};
