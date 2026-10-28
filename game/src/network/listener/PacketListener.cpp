#include "PacketListener.h"

PacketListener::PacketListener()
{}

PacketListener::~PacketListener()
{}

bool PacketListener::IsServerPacketListener()
{
    return false;
}

void PacketListener::OnUnhandledPacket(Packet *packet)
{
    (void) packet;
}

void PacketListener::OnDisconnect(eDISCONNECT_REASON reason)
{
    (void) reason;
}

void PacketListener::HandleDisconnect(DisconnectS2CPacket *packet)
{
    OnDisconnect(packet->GetReason());
}

void PacketListener::HandlePreLogin(PreLoginC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleLogin(LoginC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleLogin(LoginS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleInfo(InfoS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleLevelReady(LevelReadyS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleChunkBlocksUpdate(ChunkBlocksUpdateS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetPlayerPosition(SetPlayerPositionS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleAddPlayer(AddPlayerS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleMovePlayer(MovePlayerS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleMoveEntityPos(MoveEntityPosS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleMoveEntityRot(MoveEntityRotS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleMoveEntityPosRot(MoveEntityPosRotS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleRotateHead(RotateHeadS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetEntityMotion(SetEntityMotionS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleTeleportEntity(TeleportEntityS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSound(SoundS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleStopSound(StopSoundS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleParticle(ParticleS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetGameMode(SetGameModeS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetHealth(SetHealthS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetExperience(SetExperienceS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleContainerOpen(ContainerOpenS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleContainerClose(ContainerCloseS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleRemovePlayer(RemovePlayerS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleDisconnect(DisconnectC2SPacket *packet)
{
    OnDisconnect(packet->GetReason());
}

void PacketListener::HandleMovePlayer(MovePlayerC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleBlockUpdate(BlockUpdateS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandlePlayerAction(PlayerActionC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSound(SoundC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleKeepAlive(KeepAliveS2CPacket *packet)
{
    (void) packet;
}

void PacketListener::HandleKeepAlive(KeepAliveC2SPacket *packet)
{
    (void) packet;
}

void PacketListener::HandleCustomPayload(CustomPayloadS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleCustomPayload(CustomPayloadC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleChat(ChatS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleChat(ChatC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSwing(SwingC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSwing(SwingS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleBlockDestruction(BlockDestructionS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleChestOpen(ChestOpenS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandlePlayerSleep(PlayerSleepS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSpawnerData(SpawnerDataS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSignData(SignDataS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleMovingPiston(MovingPistonS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleTeams(TeamsS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleWorldEffect(WorldEffectS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleOpenSignEditor(OpenSignEditorS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSignUpdate(SignUpdateC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleRespawn(RespawnC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleChunkAck(ChunkAckC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandlePlayerDeath(PlayerDeathS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleAnimateHurt(AnimateHurtS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleAnimateCrit(AnimateCritS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleAttack(AttackC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleInteract(InteractC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleKnockback(KnockbackS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleExplode(ExplodeS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleTabComplete(TabCompleteC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleTabComplete(TabCompleteS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleMultiBlockUpdate(MultiBlockUpdateS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleUpdateAttributes(UpdateAttributesS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetCarriedItem(SetCarriedItemC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleContainerClick(ContainerClickC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandlePickBlock(PickBlockC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleContainerSetContent(ContainerSetContentS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleContainerSetData(ContainerSetDataS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleContainerSetSlot(ContainerSetSlotS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetEquipment(SetEquipmentS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleServerStatus(ServerStatusS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleChangeDimension(ChangeDimensionS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleDynamicLight(DynamicLightS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleDynamicLight(DynamicLightC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleFogVolume(FogVolumeS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleFogVolume(FogVolumeC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleAtmosphere(AtmosphereS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleAtmosphere(AtmosphereC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleDynamicLightMenu(DynamicLightMenuS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetTime(SetTimeS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleTickingState(TickingStateS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleTickingStep(TickingStepS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleStickFightScores(StickFightScoresS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetWeather(SetWeatherS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleWorldBorder(WorldBorderS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleAddEntity(AddEntityS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleTakeItemEntity(TakeItemEntityS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleRemoveEntities(RemoveEntitiesS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetEntityData(SetEntityDataS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetMobData(SetMobDataS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleEntityEvent(EntityEventS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetPassengers(SetPassengersS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleMoveVehicle(MoveVehicleC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandlePlayerInput(PlayerInputC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleRenameItem(RenameItemC2SPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}

void PacketListener::HandleSetBoatData(SetBoatDataS2CPacket *packet)
{
    OnUnhandledPacket((Packet *) packet);
}
