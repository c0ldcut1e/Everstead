#include "MovePlayerS2CPacket.h"

#include "../../listener/PacketListener.h"

MovePlayerS2CPacket::MovePlayerS2CPacket()
{
    m_playerIndex = 0;
    m_x           = 0.0;
    m_y           = 0.0;
    m_z           = 0.0;
    m_yaw         = 0.0f;
    m_pitch       = 0.0f;
}

MovePlayerS2CPacket::MovePlayerS2CPacket(uint8_t playerIndex, double x, double y,
                                                         double z, float yaw, float pitch)
{
    m_playerIndex = playerIndex;
    m_x           = x;
    m_y           = y;
    m_z           = z;
    m_yaw         = yaw;
    m_pitch       = pitch;
}

MovePlayerS2CPacket::~MovePlayerS2CPacket()
{
}

ePACKET_ID MovePlayerS2CPacket::GetId()
{
    return ePacketId_MovePlayerS2C;
}

void MovePlayerS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
    m_x           = inputStream->ReadDouble();
    m_y           = inputStream->ReadDouble();
    m_z           = inputStream->ReadDouble();
    m_yaw         = inputStream->ReadFloat();
    m_pitch       = inputStream->ReadFloat();
}

void MovePlayerS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteDouble(m_x);
    outputStream->WriteDouble(m_y);
    outputStream->WriteDouble(m_z);
    outputStream->WriteFloat(m_yaw);
    outputStream->WriteFloat(m_pitch);
}

void MovePlayerS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleMovePlayer(this);
}

uint8_t MovePlayerS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

double MovePlayerS2CPacket::GetX() const
{
    return m_x;
}

double MovePlayerS2CPacket::GetY() const
{
    return m_y;
}

double MovePlayerS2CPacket::GetZ() const
{
    return m_z;
}

float MovePlayerS2CPacket::GetYaw() const
{
    return m_yaw;
}

float MovePlayerS2CPacket::GetPitch() const
{
    return m_pitch;
}
