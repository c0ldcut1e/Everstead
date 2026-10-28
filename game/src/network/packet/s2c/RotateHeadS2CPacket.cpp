#include "RotateHeadS2CPacket.h"

#include "../../listener/PacketListener.h"

RotateHeadS2CPacket::RotateHeadS2CPacket()
{
    m_entityId = 0;
    m_headYaw  = 0;
}

RotateHeadS2CPacket::RotateHeadS2CPacket(int entityId, uint8_t headYaw)
{
    m_entityId = entityId;
    m_headYaw  = headYaw;
}

RotateHeadS2CPacket::~RotateHeadS2CPacket()
{}

ePACKET_ID RotateHeadS2CPacket::GetId()
{
    return ePacketId_RotateHeadS2C;
}

void RotateHeadS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    m_headYaw  = inputStream->ReadByte();
}

void RotateHeadS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteByte(m_headYaw);
}

void RotateHeadS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleRotateHead(this);
}

int RotateHeadS2CPacket::GetEntityId() const
{
    return m_entityId;
}

uint8_t RotateHeadS2CPacket::GetHeadYaw() const
{
    return m_headYaw;
}
