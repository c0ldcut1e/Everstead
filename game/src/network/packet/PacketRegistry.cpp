#include "PacketRegistry.h"

#include "../../io/ByteArrayInputStream.h"
#include "../../io/ByteArrayOutputStream.h"
#include "Packet.h"
#include "c2s/AtmosphereC2SPacket.h"
#include "c2s/AttackC2SPacket.h"
#include "c2s/ChatC2SPacket.h"
#include "c2s/ChunkAckC2SPacket.h"
#include "c2s/ContainerClickC2SPacket.h"
#include "c2s/CustomPayloadC2SPacket.h"
#include "c2s/DisconnectC2SPacket.h"
#include "c2s/DynamicLightC2SPacket.h"
#include "c2s/FogVolumeC2SPacket.h"
#include "c2s/InteractC2SPacket.h"
#include "c2s/KeepAliveC2SPacket.h"
#include "c2s/LoginC2SPacket.h"
#include "c2s/MovePlayerC2SPacket.h"
#include "c2s/MoveVehicleC2SPacket.h"
#include "c2s/PickBlockC2SPacket.h"
#include "c2s/PlayerActionC2SPacket.h"
#include "c2s/PlayerInputC2SPacket.h"
#include "c2s/PreLoginC2SPacket.h"
#include "c2s/RenameItemC2SPacket.h"
#include "c2s/RespawnC2SPacket.h"
#include "c2s/SetCarriedItemC2SPacket.h"
#include "c2s/SignUpdateC2SPacket.h"
#include "c2s/SoundC2SPacket.h"
#include "c2s/SwingC2SPacket.h"
#include "c2s/TabCompleteC2SPacket.h"
#include "s2c/AddEntityS2CPacket.h"
#include "s2c/AddPlayerS2CPacket.h"
#include "s2c/AnimateCritS2CPacket.h"
#include "s2c/AnimateHurtS2CPacket.h"
#include "s2c/AtmosphereS2CPacket.h"
#include "s2c/BlockDestructionS2CPacket.h"
#include "s2c/BlockUpdateS2CPacket.h"
#include "s2c/ChangeDimensionS2CPacket.h"
#include "s2c/ChatS2CPacket.h"
#include "s2c/ChestOpenS2CPacket.h"
#include "s2c/ChunkBlocksUpdateS2CPacket.h"
#include "s2c/ContainerCloseS2CPacket.h"
#include "s2c/ContainerOpenS2CPacket.h"
#include "s2c/ContainerSetContentS2CPacket.h"
#include "s2c/ContainerSetDataS2CPacket.h"
#include "s2c/ContainerSetSlotS2CPacket.h"
#include "s2c/CustomPayloadS2CPacket.h"
#include "s2c/DisconnectS2CPacket.h"
#include "s2c/DynamicLightMenuS2CPacket.h"
#include "s2c/DynamicLightS2CPacket.h"
#include "s2c/EntityEventS2CPacket.h"
#include "s2c/ExplodeS2CPacket.h"
#include "s2c/FogVolumeS2CPacket.h"
#include "s2c/InfoS2CPacket.h"
#include "s2c/KeepAliveS2CPacket.h"
#include "s2c/KnockbackS2CPacket.h"
#include "s2c/LevelReadyS2CPacket.h"
#include "s2c/LoginS2CPacket.h"
#include "s2c/MoveEntityPosRotS2CPacket.h"
#include "s2c/MoveEntityPosS2CPacket.h"
#include "s2c/MoveEntityRotS2CPacket.h"
#include "s2c/MovePlayerS2CPacket.h"
#include "s2c/MovingPistonS2CPacket.h"
#include "s2c/MultiBlockUpdateS2CPacket.h"
#include "s2c/OpenSignEditorS2CPacket.h"
#include "s2c/ParticleS2CPacket.h"
#include "s2c/PlayerDeathS2CPacket.h"
#include "s2c/PlayerSleepS2CPacket.h"
#include "s2c/RemoveEntitiesS2CPacket.h"
#include "s2c/RemovePlayerS2CPacket.h"
#include "s2c/RotateHeadS2CPacket.h"
#include "s2c/ServerStatusS2CPacket.h"
#include "s2c/SetBoatDataS2CPacket.h"
#include "s2c/SetEntityDataS2CPacket.h"
#include "s2c/SetEntityMotionS2CPacket.h"
#include "s2c/SetEquipmentS2CPacket.h"
#include "s2c/SetExperienceS2CPacket.h"
#include "s2c/SetGameModeS2CPacket.h"
#include "s2c/SetHealthS2CPacket.h"
#include "s2c/SetMobDataS2CPacket.h"
#include "s2c/SetPassengersS2CPacket.h"
#include "s2c/SetPlayerPositionS2CPacket.h"
#include "s2c/SetTimeS2CPacket.h"
#include "s2c/SetWeatherS2CPacket.h"
#include "s2c/SignDataS2CPacket.h"
#include "s2c/SoundS2CPacket.h"
#include "s2c/SpawnerDataS2CPacket.h"
#include "s2c/StickFightScoresS2CPacket.h"
#include "s2c/StopSoundS2CPacket.h"
#include "s2c/SwingS2CPacket.h"
#include "s2c/TabCompleteS2CPacket.h"
#include "s2c/TakeItemEntityS2CPacket.h"
#include "s2c/TeamsS2CPacket.h"
#include "s2c/TeleportEntityS2CPacket.h"
#include "s2c/TickingStateS2CPacket.h"
#include "s2c/TickingStepS2CPacket.h"
#include "s2c/UpdateAttributesS2CPacket.h"
#include "s2c/WorldBorderS2CPacket.h"
#include "s2c/WorldEffectS2CPacket.h"

PacketRegistry::PacketRegistry()
{
    Map(ePacketId_LoginS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new LoginS2CPacket());
        });
    Map(ePacketId_InfoS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new InfoS2CPacket());
        });
    Map(ePacketId_LevelReadyS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new LevelReadyS2CPacket());
        });
    Map(ePacketId_ChunkBlocksUpdateS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ChunkBlocksUpdateS2CPacket());
        });
    Map(ePacketId_SetPlayerPositionS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetPlayerPositionS2CPacket());
        });
    Map(ePacketId_AddPlayerS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new AddPlayerS2CPacket());
        });
    Map(ePacketId_MovePlayerS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new MovePlayerS2CPacket());
        });
    Map(ePacketId_RotateHeadS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new RotateHeadS2CPacket());
        });
    Map(ePacketId_SetEntityMotionS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetEntityMotionS2CPacket());
        });
    Map(ePacketId_MoveEntityPosS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new MoveEntityPosS2CPacket());
        });
    Map(ePacketId_MoveEntityRotS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new MoveEntityRotS2CPacket());
        });
    Map(ePacketId_MoveEntityPosRotS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new MoveEntityPosRotS2CPacket());
        });
    Map(ePacketId_TeleportEntityS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new TeleportEntityS2CPacket());
        });
    Map(ePacketId_SoundS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SoundS2CPacket());
        });
    Map(ePacketId_StopSoundS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new StopSoundS2CPacket());
        });
    Map(ePacketId_ParticleS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ParticleS2CPacket());
        });
    Map(ePacketId_SetGameModeS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetGameModeS2CPacket());
        });
    Map(ePacketId_SetHealthS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetHealthS2CPacket());
        });
    Map(ePacketId_SetExperienceS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetExperienceS2CPacket());
        });
    Map(ePacketId_RemovePlayerS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new RemovePlayerS2CPacket());
        });
    Map(ePacketId_KeepAliveS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new KeepAliveS2CPacket());
        });
    Map(ePacketId_BlockUpdateS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new BlockUpdateS2CPacket());
        });
    Map(ePacketId_DisconnectS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new DisconnectS2CPacket());
        });
    Map(ePacketId_CustomPayloadS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new CustomPayloadS2CPacket());
        });
    Map(ePacketId_ChatS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ChatS2CPacket());
        });
    Map(ePacketId_SwingS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SwingS2CPacket());
        });
    Map(ePacketId_BlockDestructionS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new BlockDestructionS2CPacket());
        });
    Map(ePacketId_AnimateHurtS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new AnimateHurtS2CPacket());
        });
    Map(ePacketId_AnimateCritS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new AnimateCritS2CPacket());
        });
    Map(ePacketId_PlayerDeathS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new PlayerDeathS2CPacket());
        });
    Map(ePacketId_ExplodeS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ExplodeS2CPacket());
        });
    Map(ePacketId_KnockbackS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new KnockbackS2CPacket());
        });
    Map(ePacketId_ChestOpenS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ChestOpenS2CPacket());
        });
    Map(ePacketId_PlayerSleepS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new PlayerSleepS2CPacket());
        });
    Map(ePacketId_SpawnerDataS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SpawnerDataS2CPacket());
        });
    Map(ePacketId_SignDataS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SignDataS2CPacket());
        });
    Map(ePacketId_MovingPistonS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new MovingPistonS2CPacket());
        });
    Map(ePacketId_TeamsS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new TeamsS2CPacket());
        });
    Map(ePacketId_WorldEffectS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new WorldEffectS2CPacket());
        });
    Map(ePacketId_OpenSignEditorS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new OpenSignEditorS2CPacket());
        });
    Map(ePacketId_SignUpdateC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new SignUpdateC2SPacket());
        });
    Map(ePacketId_ContainerOpenS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ContainerOpenS2CPacket());
        });
    Map(ePacketId_ContainerCloseS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ContainerCloseS2CPacket());
        });
    Map(ePacketId_ContainerSetDataS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ContainerSetDataS2CPacket());
        });
    Map(ePacketId_ServerStatusS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ServerStatusS2CPacket());
        });
    Map(ePacketId_ChangeDimensionS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ChangeDimensionS2CPacket());
        });
    Map(ePacketId_DynamicLightS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new DynamicLightS2CPacket());
        });
    Map(ePacketId_FogVolumeS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new FogVolumeS2CPacket());
        });
    Map(ePacketId_AtmosphereS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new AtmosphereS2CPacket());
        });
    Map(ePacketId_DynamicLightMenuS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new DynamicLightMenuS2CPacket());
        });
    Map(ePacketId_SetTimeS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetTimeS2CPacket());
        });
    Map(ePacketId_TickingStateS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new TickingStateS2CPacket());
        });
    Map(ePacketId_TickingStepS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new TickingStepS2CPacket());
        });
    Map(ePacketId_StickFightScoresS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new StickFightScoresS2CPacket());
        });
    Map(ePacketId_SetWeatherS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetWeatherS2CPacket());
        });
    Map(ePacketId_WorldBorderS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new WorldBorderS2CPacket());
        });
    Map(ePacketId_AddEntityS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new AddEntityS2CPacket());
        });
    Map(ePacketId_TakeItemEntityS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new TakeItemEntityS2CPacket());
        });
    Map(ePacketId_RemoveEntitiesS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new RemoveEntitiesS2CPacket());
        });
    Map(ePacketId_SetPassengersS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetPassengersS2CPacket());
        });
    Map(ePacketId_SetBoatDataS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetBoatDataS2CPacket());
        });
    Map(ePacketId_SetMobDataS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetMobDataS2CPacket());
        });
    Map(ePacketId_EntityEventS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new EntityEventS2CPacket());
        });
    Map(ePacketId_SetEntityDataS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetEntityDataS2CPacket());
        });
    Map(ePacketId_ContainerSetContentS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ContainerSetContentS2CPacket());
        });
    Map(ePacketId_ContainerSetSlotS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new ContainerSetSlotS2CPacket());
        });
    Map(ePacketId_SetEquipmentS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new SetEquipmentS2CPacket());
        });

    Map(ePacketId_LoginC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new LoginC2SPacket());
        });
    Map(ePacketId_PreLoginC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new PreLoginC2SPacket());
        });
    Map(ePacketId_MovePlayerC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new MovePlayerC2SPacket());
        });
    Map(ePacketId_KeepAliveC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new KeepAliveC2SPacket());
        });
    Map(ePacketId_PlayerActionC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new PlayerActionC2SPacket());
        });
    Map(ePacketId_SoundC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new SoundC2SPacket());
        });
    Map(ePacketId_SwingC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new SwingC2SPacket());
        });
    Map(ePacketId_DynamicLightC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new DynamicLightC2SPacket());
        });
    Map(ePacketId_FogVolumeC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new FogVolumeC2SPacket());
        });
    Map(ePacketId_AtmosphereC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new AtmosphereC2SPacket());
        });
    Map(ePacketId_RespawnC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new RespawnC2SPacket());
        });
    Map(ePacketId_ChunkAckC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new ChunkAckC2SPacket());
        });
    Map(ePacketId_InteractC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new InteractC2SPacket());
        });
    Map(ePacketId_MoveVehicleC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new MoveVehicleC2SPacket());
        });
    Map(ePacketId_PlayerInputC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new PlayerInputC2SPacket());
        });
    Map(ePacketId_RenameItemC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new RenameItemC2SPacket());
        });
    Map(ePacketId_AttackC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new AttackC2SPacket());
        });
    Map(ePacketId_DisconnectC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new DisconnectC2SPacket());
        });
    Map(ePacketId_CustomPayloadC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new CustomPayloadC2SPacket());
        });
    Map(ePacketId_ChatC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new ChatC2SPacket());
        });
    Map(ePacketId_SetCarriedItemC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new SetCarriedItemC2SPacket());
        });
    Map(ePacketId_PickBlockC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new PickBlockC2SPacket());
        });
    Map(ePacketId_ContainerClickC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new ContainerClickC2SPacket());
        });
    Map(ePacketId_TabCompleteC2S, false, true,
        []
        {
            return std::shared_ptr<Packet>(new TabCompleteC2SPacket());
        });
    Map(ePacketId_TabCompleteS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new TabCompleteS2CPacket());
        });
    Map(ePacketId_MultiBlockUpdateS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new MultiBlockUpdateS2CPacket());
        });
    Map(ePacketId_UpdateAttributesS2C, true, false,
        []
        {
            return std::shared_ptr<Packet>(new UpdateAttributesS2CPacket());
        });
}

PacketRegistry::~PacketRegistry()
{}

bool PacketRegistry::WritePacket(Packet *packet, std::vector<uint8_t> *outPtr)
{
    ByteArrayOutputStream byteStream;
    DataOutputStream outputStream(&byteStream);
    outputStream.WriteByte((uint8_t) packet->GetId());
    packet->Write(&outputStream);
    *outPtr = byteStream.ToByteArray();
    return true;
}

std::shared_ptr<Packet> PacketRegistry::ReadPacket(const std::vector<uint8_t> &bytes, bool serverPacketListener)
{
    if (bytes.empty())
    {
        return nullptr;
    }

    ByteArrayInputStream byteStream(bytes);
    DataInputStream inputStream(&byteStream);
    int packetId = inputStream.ReadByte();

    std::map<int, PacketCreateFunction> *packets = serverPacketListener ? &m_serverReceivedPackets : &m_clientReceivedPackets;
    auto it                                      = packets->find(packetId);
    if (it == packets->end())
    {
        return nullptr;
    }

    std::shared_ptr<Packet> packet = it->second();
    packet->Read(&inputStream);
    return packet;
}

void PacketRegistry::Map(ePACKET_ID id, bool receiveOnClient, bool receiveOnServer, const PacketCreateFunction &createFunction)
{
    if (receiveOnClient)
    {
        m_clientReceivedPackets[id] = createFunction;
    }

    if (receiveOnServer)
    {
        m_serverReceivedPackets[id] = createFunction;
    }
}
