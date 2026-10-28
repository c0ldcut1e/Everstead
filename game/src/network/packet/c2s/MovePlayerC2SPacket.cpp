#include "MovePlayerC2SPacket.h"

#include "../../listener/PacketListener.h"

MovePlayerC2SPacket::MovePlayerC2SPacket()
{
    m_x          = 0.0;
    m_y          = 0.0;
    m_z          = 0.0;
    m_yaw        = 0.0f;
    m_pitch      = 0.0f;
    m_onGround   = false;
    m_flying     = false;
    m_teleportId = 0;
}

MovePlayerC2SPacket::MovePlayerC2SPacket(double x, double y, double z, float yaw, float pitch, bool onGround, bool flying, uint32_t teleportId)
{
    m_x          = x;
    m_y          = y;
    m_z          = z;
    m_yaw        = yaw;
    m_pitch      = pitch;
    m_onGround   = onGround;
    m_flying     = flying;
    m_teleportId = teleportId;
}

MovePlayerC2SPacket::~MovePlayerC2SPacket()
{}

ePACKET_ID MovePlayerC2SPacket::GetId()
{
    return ePacketId_MovePlayerC2S;
}

void MovePlayerC2SPacket::Read(DataInputStream *inputStream)
{
    m_x          = inputStream->ReadDouble();
    m_y          = inputStream->ReadDouble();
    m_z          = inputStream->ReadDouble();
    m_yaw        = inputStream->ReadFloat();
    m_pitch      = inputStream->ReadFloat();
    m_onGround   = inputStream->ReadByte() != 0;
    m_flying     = inputStream->ReadByte() != 0;
    m_teleportId = inputStream->ReadInt();
}

void MovePlayerC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteDouble(m_x);
    outputStream->WriteDouble(m_y);
    outputStream->WriteDouble(m_z);
    outputStream->WriteFloat(m_yaw);
    outputStream->WriteFloat(m_pitch);
    outputStream->WriteByte((uint8_t) m_onGround);
    outputStream->WriteByte((uint8_t) m_flying);
    outputStream->WriteInt(m_teleportId);
}

void MovePlayerC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleMovePlayer(this);
}

double MovePlayerC2SPacket::GetX() const
{
    return m_x;
}

double MovePlayerC2SPacket::GetY() const
{
    return m_y;
}

double MovePlayerC2SPacket::GetZ() const
{
    return m_z;
}

float MovePlayerC2SPacket::GetYaw() const
{
    return m_yaw;
}

float MovePlayerC2SPacket::GetPitch() const
{
    return m_pitch;
}

bool MovePlayerC2SPacket::IsOnGround() const
{
    return m_onGround;
}

bool MovePlayerC2SPacket::IsFlying() const
{
    return m_flying;
}

uint32_t MovePlayerC2SPacket::GetTeleportId() const
{
    return m_teleportId;
}
