#include "SetPlayerPositionS2CPacket.h"

#include "../../listener/PacketListener.h"

SetPlayerPositionS2CPacket::SetPlayerPositionS2CPacket()
{
    m_x          = 0.0;
    m_y          = 0.0;
    m_z          = 0.0;
    m_yaw        = 0.0f;
    m_pitch      = 0.0f;
    m_teleportId = 0;
    m_velocityX  = 0.0f;
    m_velocityY  = 0.0f;
    m_velocityZ  = 0.0f;
}

SetPlayerPositionS2CPacket::SetPlayerPositionS2CPacket(double x, double y, double z, float yaw, float pitch, uint32_t teleportId,
                                                       const glm::dvec3 &velocity)
{
    m_x          = x;
    m_y          = y;
    m_z          = z;
    m_yaw        = yaw;
    m_pitch      = pitch;
    m_teleportId = teleportId;
    m_velocityX  = (float) velocity.x;
    m_velocityY  = (float) velocity.y;
    m_velocityZ  = (float) velocity.z;
}

SetPlayerPositionS2CPacket::~SetPlayerPositionS2CPacket()
{}

ePACKET_ID SetPlayerPositionS2CPacket::GetId()
{
    return ePacketId_SetPlayerPositionS2C;
}

void SetPlayerPositionS2CPacket::Read(DataInputStream *inputStream)
{
    m_x          = inputStream->ReadDouble();
    m_y          = inputStream->ReadDouble();
    m_z          = inputStream->ReadDouble();
    m_yaw        = inputStream->ReadFloat();
    m_pitch      = inputStream->ReadFloat();
    m_teleportId = inputStream->ReadInt();
    m_velocityX  = inputStream->ReadFloat();
    m_velocityY  = inputStream->ReadFloat();
    m_velocityZ  = inputStream->ReadFloat();
}

void SetPlayerPositionS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteDouble(m_x);
    outputStream->WriteDouble(m_y);
    outputStream->WriteDouble(m_z);
    outputStream->WriteFloat(m_yaw);
    outputStream->WriteFloat(m_pitch);
    outputStream->WriteInt(m_teleportId);
    outputStream->WriteFloat(m_velocityX);
    outputStream->WriteFloat(m_velocityY);
    outputStream->WriteFloat(m_velocityZ);
}

void SetPlayerPositionS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetPlayerPosition(this);
}

double SetPlayerPositionS2CPacket::GetX() const
{
    return m_x;
}

double SetPlayerPositionS2CPacket::GetY() const
{
    return m_y;
}

double SetPlayerPositionS2CPacket::GetZ() const
{
    return m_z;
}

float SetPlayerPositionS2CPacket::GetYaw() const
{
    return m_yaw;
}

float SetPlayerPositionS2CPacket::GetPitch() const
{
    return m_pitch;
}

uint32_t SetPlayerPositionS2CPacket::GetTeleportId() const
{
    return m_teleportId;
}

glm::dvec3 SetPlayerPositionS2CPacket::GetVelocity() const
{
    return glm::dvec3(m_velocityX, m_velocityY, m_velocityZ);
}
