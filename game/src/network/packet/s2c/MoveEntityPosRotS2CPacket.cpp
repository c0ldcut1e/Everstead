#include "MoveEntityPosRotS2CPacket.h"

#include "../../listener/PacketListener.h"

MoveEntityPosRotS2CPacket::MoveEntityPosRotS2CPacket()
{
    m_entityId = 0;
    m_xa       = 0;
    m_ya       = 0;
    m_za       = 0;
    m_yaw      = 0;
    m_pitch    = 0;
    m_onGround = false;
}

MoveEntityPosRotS2CPacket::MoveEntityPosRotS2CPacket(int entityId, int16_t xa, int16_t ya, int16_t za, uint8_t yaw, uint8_t pitch, bool onGround)
{
    m_entityId = entityId;
    m_xa       = xa;
    m_ya       = ya;
    m_za       = za;
    m_yaw      = yaw;
    m_pitch    = pitch;
    m_onGround = onGround;
}

MoveEntityPosRotS2CPacket::~MoveEntityPosRotS2CPacket()
{}

ePACKET_ID MoveEntityPosRotS2CPacket::GetId()
{
    return ePacketId_MoveEntityPosRotS2C;
}

void MoveEntityPosRotS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    m_xa       = (int16_t) inputStream->ReadShort();
    m_ya       = (int16_t) inputStream->ReadShort();
    m_za       = (int16_t) inputStream->ReadShort();
    m_yaw      = inputStream->ReadByte();
    m_pitch    = inputStream->ReadByte();
    m_onGround = inputStream->ReadBool();
}

void MoveEntityPosRotS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteShort((uint16_t) m_xa);
    outputStream->WriteShort((uint16_t) m_ya);
    outputStream->WriteShort((uint16_t) m_za);
    outputStream->WriteByte(m_yaw);
    outputStream->WriteByte(m_pitch);
    outputStream->WriteByte((uint8_t) m_onGround);
}

void MoveEntityPosRotS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleMoveEntityPosRot(this);
}

int MoveEntityPosRotS2CPacket::GetEntityId() const
{
    return m_entityId;
}

int16_t MoveEntityPosRotS2CPacket::GetXa() const
{
    return m_xa;
}

int16_t MoveEntityPosRotS2CPacket::GetYa() const
{
    return m_ya;
}

int16_t MoveEntityPosRotS2CPacket::GetZa() const
{
    return m_za;
}

int16_t MoveEntityPosRotS2CPacket::GetYaw() const
{
    return m_yaw;
}

uint8_t MoveEntityPosRotS2CPacket::GetPitch() const
{
    return m_pitch;
}

bool MoveEntityPosRotS2CPacket::IsOnGround() const
{
    return m_onGround;
}
