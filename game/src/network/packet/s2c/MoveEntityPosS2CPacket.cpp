#include "MoveEntityPosS2CPacket.h"

#include "../../listener/PacketListener.h"

MoveEntityPosS2CPacket::MoveEntityPosS2CPacket()
{
    m_entityId = 0;
    m_xa       = 0;
    m_ya       = 0;
    m_za       = 0;
    m_onGround = false;
}

MoveEntityPosS2CPacket::MoveEntityPosS2CPacket(int entityId, int16_t xa, int16_t ya, int16_t za, bool onGround)
{
    m_entityId = entityId;
    m_xa       = xa;
    m_ya       = ya;
    m_za       = za;
    m_onGround = onGround;
}

MoveEntityPosS2CPacket::~MoveEntityPosS2CPacket()
{}

ePACKET_ID MoveEntityPosS2CPacket::GetId()
{
    return ePacketId_MoveEntityPosS2C;
}

void MoveEntityPosS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    m_xa       = (int16_t) inputStream->ReadShort();
    m_ya       = (int16_t) inputStream->ReadShort();
    m_za       = (int16_t) inputStream->ReadShort();
    m_onGround = inputStream->ReadBool();
}

void MoveEntityPosS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteShort((uint16_t) m_xa);
    outputStream->WriteShort((uint16_t) m_ya);
    outputStream->WriteShort((uint16_t) m_za);
    outputStream->WriteByte((uint8_t) m_onGround);
}

void MoveEntityPosS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleMoveEntityPos(this);
}

int MoveEntityPosS2CPacket::GetEntityId() const
{
    return m_entityId;
}

int16_t MoveEntityPosS2CPacket::GetXa() const
{
    return m_xa;
}

int16_t MoveEntityPosS2CPacket::GetYa() const
{
    return m_ya;
}

int16_t MoveEntityPosS2CPacket::GetZa() const
{
    return m_za;
}

bool MoveEntityPosS2CPacket::IsOnGround() const
{
    return m_onGround;
}
