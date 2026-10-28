#include "TeleportEntityS2CPacket.h"

#include "../../listener/PacketListener.h"

TeleportEntityS2CPacket::TeleportEntityS2CPacket()
{
    m_entityId = 0;
    m_x        = 0;
    m_y        = 0;
    m_z        = 0;
    m_yaw      = 0;
    m_pitch    = 0;
    m_onGround = false;
}

TeleportEntityS2CPacket::TeleportEntityS2CPacket(int entityId, int64_t x, int64_t y, int64_t z, uint8_t yaw, uint8_t pitch, bool onGround)
{
    m_entityId = entityId;
    m_x        = x;
    m_y        = y;
    m_z        = z;
    m_yaw      = yaw;
    m_pitch    = pitch;
    m_onGround = onGround;
}

TeleportEntityS2CPacket::~TeleportEntityS2CPacket()
{}

ePACKET_ID TeleportEntityS2CPacket::GetId()
{
    return ePacketId_TeleportEntityS2C;
}

void TeleportEntityS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    m_x        = (int64_t) inputStream->ReadLong();
    m_y        = (int64_t) inputStream->ReadLong();
    m_z        = (int64_t) inputStream->ReadLong();
    m_yaw      = inputStream->ReadByte();
    m_pitch    = inputStream->ReadByte();
    m_onGround = inputStream->ReadBool();
}

void TeleportEntityS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteLong((uint64_t) m_x);
    outputStream->WriteLong((uint64_t) m_y);
    outputStream->WriteLong((uint64_t) m_z);
    outputStream->WriteByte(m_yaw);
    outputStream->WriteByte(m_pitch);
    outputStream->WriteByte((uint8_t) m_onGround);
}

void TeleportEntityS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleTeleportEntity(this);
}

int TeleportEntityS2CPacket::GetEntityId() const
{
    return m_entityId;
}

int64_t TeleportEntityS2CPacket::GetX() const
{
    return m_x;
}

int64_t TeleportEntityS2CPacket::GetY() const
{
    return m_y;
}

int64_t TeleportEntityS2CPacket::GetZ() const
{
    return m_z;
}

uint8_t TeleportEntityS2CPacket::GetYaw() const
{
    return m_yaw;
}

uint8_t TeleportEntityS2CPacket::GetPitch() const
{
    return m_pitch;
}

bool TeleportEntityS2CPacket::IsOnGround() const
{
    return m_onGround;
}
