#include "MoveEntityRotS2CPacket.h"

#include "../../listener/PacketListener.h"

MoveEntityRotS2CPacket::MoveEntityRotS2CPacket()
{
    m_entityId = 0;
    m_yaw      = 0;
    m_pitch    = 0;
}

MoveEntityRotS2CPacket::MoveEntityRotS2CPacket(int entityId, uint8_t yaw, uint8_t pitch)
{
    m_entityId = entityId;
    m_yaw      = yaw;
    m_pitch    = pitch;
}

MoveEntityRotS2CPacket::~MoveEntityRotS2CPacket()
{}

ePACKET_ID MoveEntityRotS2CPacket::GetId()
{
    return ePacketId_MoveEntityRotS2C;
}

void MoveEntityRotS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    m_yaw      = inputStream->ReadByte();
    m_pitch    = inputStream->ReadByte();
}

void MoveEntityRotS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteByte(m_yaw);
    outputStream->WriteByte(m_pitch);
}

void MoveEntityRotS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleMoveEntityRot(this);
}

int MoveEntityRotS2CPacket::GetEntityId() const
{
    return m_entityId;
}

uint8_t MoveEntityRotS2CPacket::GetYaw() const
{
    return m_yaw;
}

uint8_t MoveEntityRotS2CPacket::GetPitch() const
{
    return m_pitch;
}
