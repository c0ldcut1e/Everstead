#pragma once

#include "../packet/Packet.h"
#include "../packet/c2s/AtmosphereC2SPacket.h"
#include "../packet/c2s/AttackC2SPacket.h"
#include "../packet/c2s/ChatC2SPacket.h"
#include "../packet/c2s/ChunkAckC2SPacket.h"
#include "../packet/c2s/ContainerClickC2SPacket.h"
#include "../packet/c2s/CustomPayloadC2SPacket.h"
#include "../packet/c2s/DisconnectC2SPacket.h"
#include "../packet/c2s/DynamicLightC2SPacket.h"
#include "../packet/c2s/FogVolumeC2SPacket.h"
#include "../packet/c2s/InteractC2SPacket.h"
#include "../packet/c2s/KeepAliveC2SPacket.h"
#include "../packet/c2s/LoginC2SPacket.h"
#include "../packet/c2s/MovePlayerC2SPacket.h"
#include "../packet/c2s/MoveVehicleC2SPacket.h"
#include "../packet/c2s/PickBlockC2SPacket.h"
#include "../packet/c2s/PlayerActionC2SPacket.h"
#include "../packet/c2s/PlayerInputC2SPacket.h"
#include "../packet/c2s/PreLoginC2SPacket.h"
#include "../packet/c2s/RenameItemC2SPacket.h"
#include "../packet/c2s/RespawnC2SPacket.h"
#include "../packet/c2s/SetCarriedItemC2SPacket.h"
#include "../packet/c2s/SignUpdateC2SPacket.h"
#include "../packet/c2s/SoundC2SPacket.h"
#include "../packet/c2s/SwingC2SPacket.h"
#include "../packet/c2s/TabCompleteC2SPacket.h"
#include "../packet/s2c/AddEntityS2CPacket.h"
#include "../packet/s2c/AddPlayerS2CPacket.h"
#include "../packet/s2c/AnimateCritS2CPacket.h"
#include "../packet/s2c/AnimateHurtS2CPacket.h"
#include "../packet/s2c/AtmosphereS2CPacket.h"
#include "../packet/s2c/BlockDestructionS2CPacket.h"
#include "../packet/s2c/BlockUpdateS2CPacket.h"
#include "../packet/s2c/ChangeDimensionS2CPacket.h"
#include "../packet/s2c/ChatS2CPacket.h"
#include "../packet/s2c/ChestOpenS2CPacket.h"
#include "../packet/s2c/ChunkBlocksUpdateS2CPacket.h"
#include "../packet/s2c/ContainerCloseS2CPacket.h"
#include "../packet/s2c/ContainerOpenS2CPacket.h"
#include "../packet/s2c/ContainerSetContentS2CPacket.h"
#include "../packet/s2c/ContainerSetDataS2CPacket.h"
#include "../packet/s2c/ContainerSetSlotS2CPacket.h"
#include "../packet/s2c/CustomPayloadS2CPacket.h"
#include "../packet/s2c/DisconnectS2CPacket.h"
#include "../packet/s2c/DynamicLightMenuS2CPacket.h"
#include "../packet/s2c/DynamicLightS2CPacket.h"
#include "../packet/s2c/EntityEventS2CPacket.h"
#include "../packet/s2c/ExplodeS2CPacket.h"
#include "../packet/s2c/FogVolumeS2CPacket.h"
#include "../packet/s2c/InfoS2CPacket.h"
#include "../packet/s2c/KeepAliveS2CPacket.h"
#include "../packet/s2c/KnockbackS2CPacket.h"
#include "../packet/s2c/LevelReadyS2CPacket.h"
#include "../packet/s2c/LoginS2CPacket.h"
#include "../packet/s2c/MoveEntityPosRotS2CPacket.h"
#include "../packet/s2c/MoveEntityPosS2CPacket.h"
#include "../packet/s2c/MoveEntityRotS2CPacket.h"
#include "../packet/s2c/MovePlayerS2CPacket.h"
#include "../packet/s2c/MovingPistonS2CPacket.h"
#include "../packet/s2c/MultiBlockUpdateS2CPacket.h"
#include "../packet/s2c/OpenSignEditorS2CPacket.h"
#include "../packet/s2c/ParticleS2CPacket.h"
#include "../packet/s2c/PlayerDeathS2CPacket.h"
#include "../packet/s2c/PlayerSleepS2CPacket.h"
#include "../packet/s2c/RemoveEntitiesS2CPacket.h"
#include "../packet/s2c/RemovePlayerS2CPacket.h"
#include "../packet/s2c/RotateHeadS2CPacket.h"
#include "../packet/s2c/ServerStatusS2CPacket.h"
#include "../packet/s2c/SetBoatDataS2CPacket.h"
#include "../packet/s2c/SetEntityDataS2CPacket.h"
#include "../packet/s2c/SetEntityMotionS2CPacket.h"
#include "../packet/s2c/SetEquipmentS2CPacket.h"
#include "../packet/s2c/SetExperienceS2CPacket.h"
#include "../packet/s2c/SetGameModeS2CPacket.h"
#include "../packet/s2c/SetHealthS2CPacket.h"
#include "../packet/s2c/SetMobDataS2CPacket.h"
#include "../packet/s2c/SetPassengersS2CPacket.h"
#include "../packet/s2c/SetPlayerPositionS2CPacket.h"
#include "../packet/s2c/SetTimeS2CPacket.h"
#include "../packet/s2c/SetWeatherS2CPacket.h"
#include "../packet/s2c/SignDataS2CPacket.h"
#include "../packet/s2c/SoundS2CPacket.h"
#include "../packet/s2c/SpawnerDataS2CPacket.h"
#include "../packet/s2c/StickFightScoresS2CPacket.h"
#include "../packet/s2c/StopSoundS2CPacket.h"
#include "../packet/s2c/SwingS2CPacket.h"
#include "../packet/s2c/TabCompleteS2CPacket.h"
#include "../packet/s2c/TakeItemEntityS2CPacket.h"
#include "../packet/s2c/TeamsS2CPacket.h"
#include "../packet/s2c/TeleportEntityS2CPacket.h"
#include "../packet/s2c/TickingStateS2CPacket.h"
#include "../packet/s2c/TickingStepS2CPacket.h"
#include "../packet/s2c/UpdateAttributesS2CPacket.h"
#include "../packet/s2c/WorldBorderS2CPacket.h"
#include "../packet/s2c/WorldEffectS2CPacket.h"

class PacketListener
{
public:
    PacketListener();
    virtual ~PacketListener();

    virtual bool IsServerPacketListener();
    virtual void OnUnhandledPacket(Packet *packet);
    virtual void OnDisconnect(eDISCONNECT_REASON reason);
    virtual void HandleDisconnect(DisconnectS2CPacket *packet);
    virtual void HandlePreLogin(PreLoginC2SPacket *packet);
    virtual void HandleLogin(LoginC2SPacket *packet);
    virtual void HandleLogin(LoginS2CPacket *packet);
    virtual void HandleInfo(InfoS2CPacket *packet);
    virtual void HandleLevelReady(LevelReadyS2CPacket *packet);
    virtual void HandleChunkBlocksUpdate(ChunkBlocksUpdateS2CPacket *packet);
    virtual void HandleSetPlayerPosition(SetPlayerPositionS2CPacket *packet);
    virtual void HandleAddPlayer(AddPlayerS2CPacket *packet);
    virtual void HandleMovePlayer(MovePlayerS2CPacket *packet);
    virtual void HandleMoveEntityPos(MoveEntityPosS2CPacket *packet);
    virtual void HandleMoveEntityRot(MoveEntityRotS2CPacket *packet);
    virtual void HandleMoveEntityPosRot(MoveEntityPosRotS2CPacket *packet);
    virtual void HandleRotateHead(RotateHeadS2CPacket *packet);
    virtual void HandleSetEntityMotion(SetEntityMotionS2CPacket *packet);
    virtual void HandleTeleportEntity(TeleportEntityS2CPacket *packet);
    virtual void HandleSound(SoundS2CPacket *packet);
    virtual void HandleStopSound(StopSoundS2CPacket *packet);
    virtual void HandleParticle(ParticleS2CPacket *packet);
    virtual void HandleSetGameMode(SetGameModeS2CPacket *packet);
    virtual void HandleSetHealth(SetHealthS2CPacket *packet);
    virtual void HandleSetExperience(SetExperienceS2CPacket *packet);
    virtual void HandleContainerOpen(ContainerOpenS2CPacket *packet);
    virtual void HandleContainerClose(ContainerCloseS2CPacket *packet);
    virtual void HandleRemovePlayer(RemovePlayerS2CPacket *packet);
    virtual void HandleDisconnect(DisconnectC2SPacket *packet);
    virtual void HandleMovePlayer(MovePlayerC2SPacket *packet);
    virtual void HandleBlockUpdate(BlockUpdateS2CPacket *packet);
    virtual void HandlePlayerAction(PlayerActionC2SPacket *packet);
    virtual void HandleSound(SoundC2SPacket *packet);
    virtual void HandleKeepAlive(KeepAliveS2CPacket *packet);
    virtual void HandleKeepAlive(KeepAliveC2SPacket *packet);
    virtual void HandleCustomPayload(CustomPayloadS2CPacket *packet);
    virtual void HandleCustomPayload(CustomPayloadC2SPacket *packet);
    virtual void HandleChat(ChatS2CPacket *packet);
    virtual void HandleChat(ChatC2SPacket *packet);
    virtual void HandleSwing(SwingC2SPacket *packet);
    virtual void HandleSwing(SwingS2CPacket *packet);
    virtual void HandleBlockDestruction(BlockDestructionS2CPacket *packet);
    virtual void HandleChestOpen(ChestOpenS2CPacket *packet);
    virtual void HandlePlayerSleep(PlayerSleepS2CPacket *packet);
    virtual void HandleSpawnerData(SpawnerDataS2CPacket *packet);
    virtual void HandleSignData(SignDataS2CPacket *packet);
    virtual void HandleMovingPiston(MovingPistonS2CPacket *packet);
    virtual void HandleTeams(TeamsS2CPacket *packet);
    virtual void HandleWorldEffect(WorldEffectS2CPacket *packet);
    virtual void HandleOpenSignEditor(OpenSignEditorS2CPacket *packet);
    virtual void HandleSignUpdate(SignUpdateC2SPacket *packet);
    virtual void HandleRespawn(RespawnC2SPacket *packet);
    virtual void HandleChunkAck(ChunkAckC2SPacket *packet);
    virtual void HandlePlayerDeath(PlayerDeathS2CPacket *packet);
    virtual void HandleAnimateHurt(AnimateHurtS2CPacket *packet);
    virtual void HandleAnimateCrit(AnimateCritS2CPacket *packet);
    virtual void HandleAttack(AttackC2SPacket *packet);
    virtual void HandleInteract(InteractC2SPacket *packet);
    virtual void HandleKnockback(KnockbackS2CPacket *packet);
    virtual void HandleExplode(ExplodeS2CPacket *packet);
    virtual void HandleTabComplete(TabCompleteC2SPacket *packet);
    virtual void HandleTabComplete(TabCompleteS2CPacket *packet);
    virtual void HandleMultiBlockUpdate(MultiBlockUpdateS2CPacket *packet);
    virtual void HandleUpdateAttributes(UpdateAttributesS2CPacket *packet);
    virtual void HandleSetCarriedItem(SetCarriedItemC2SPacket *packet);
    virtual void HandleContainerClick(ContainerClickC2SPacket *packet);
    virtual void HandlePickBlock(PickBlockC2SPacket *packet);
    virtual void HandleContainerSetContent(ContainerSetContentS2CPacket *packet);
    virtual void HandleContainerSetData(ContainerSetDataS2CPacket *packet);
    virtual void HandleContainerSetSlot(ContainerSetSlotS2CPacket *packet);
    virtual void HandleSetEquipment(SetEquipmentS2CPacket *packet);
    virtual void HandleServerStatus(ServerStatusS2CPacket *packet);
    virtual void HandleChangeDimension(ChangeDimensionS2CPacket *packet);
    virtual void HandleDynamicLight(DynamicLightS2CPacket *packet);
    virtual void HandleDynamicLight(DynamicLightC2SPacket *packet);
    virtual void HandleFogVolume(FogVolumeS2CPacket *packet);
    virtual void HandleFogVolume(FogVolumeC2SPacket *packet);
    virtual void HandleAtmosphere(AtmosphereS2CPacket *packet);
    virtual void HandleAtmosphere(AtmosphereC2SPacket *packet);
    virtual void HandleDynamicLightMenu(DynamicLightMenuS2CPacket *packet);
    virtual void HandleSetTime(SetTimeS2CPacket *packet);
    virtual void HandleTickingState(TickingStateS2CPacket *packet);
    virtual void HandleTickingStep(TickingStepS2CPacket *packet);
    virtual void HandleStickFightScores(StickFightScoresS2CPacket *packet);
    virtual void HandleSetWeather(SetWeatherS2CPacket *packet);
    virtual void HandleWorldBorder(WorldBorderS2CPacket *packet);
    virtual void HandleAddEntity(AddEntityS2CPacket *packet);
    virtual void HandleTakeItemEntity(TakeItemEntityS2CPacket *packet);
    virtual void HandleRemoveEntities(RemoveEntitiesS2CPacket *packet);
    virtual void HandleSetEntityData(SetEntityDataS2CPacket *packet);
    virtual void HandleSetMobData(SetMobDataS2CPacket *packet);
    virtual void HandleEntityEvent(EntityEventS2CPacket *packet);
    virtual void HandleSetPassengers(SetPassengersS2CPacket *packet);
    virtual void HandleMoveVehicle(MoveVehicleC2SPacket *packet);
    virtual void HandlePlayerInput(PlayerInputC2SPacket *packet);
    virtual void HandleRenameItem(RenameItemC2SPacket *packet);
    virtual void HandleSetBoatData(SetBoatDataS2CPacket *packet);
};
