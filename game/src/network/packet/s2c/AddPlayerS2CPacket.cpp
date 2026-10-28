#include "AddPlayerS2CPacket.h"

#include "../../listener/PacketListener.h"

AddPlayerS2CPacket::AddPlayerS2CPacket()
{
    m_playerIndex = 0;
    m_entityId    = 0;
    m_username    = "";
    m_x           = 0.0;
    m_y           = 0.0;
    m_z           = 0.0;
    m_yaw         = 0.0f;
    m_pitch       = 0.0f;
    m_flags       = 0;
}

AddPlayerS2CPacket::AddPlayerS2CPacket(uint8_t playerIndex, int entityId, const std::string &username, double x, double y, double z, float yaw,
                                       float pitch, uint8_t flags)
{
    m_playerIndex = playerIndex;
    m_entityId    = entityId;
    m_username    = username;
    m_x           = x;
    m_y           = y;
    m_z           = z;
    m_yaw         = yaw;
    m_pitch       = pitch;
    m_flags       = flags;
}

AddPlayerS2CPacket::~AddPlayerS2CPacket()
{}

ePACKET_ID AddPlayerS2CPacket::GetId()
{
    return ePacketId_AddPlayerS2C;
}

void AddPlayerS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
    m_entityId    = (int) inputStream->ReadInt();
    m_username    = inputStream->ReadString();
    m_x           = inputStream->ReadDouble();
    m_y           = inputStream->ReadDouble();
    m_z           = inputStream->ReadDouble();
    m_yaw         = inputStream->ReadFloat();
    m_pitch       = inputStream->ReadFloat();
    m_flags       = inputStream->ReadByte();
}

void AddPlayerS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteString(m_username);
    outputStream->WriteDouble(m_x);
    outputStream->WriteDouble(m_y);
    outputStream->WriteDouble(m_z);
    outputStream->WriteFloat(m_yaw);
    outputStream->WriteFloat(m_pitch);
    outputStream->WriteByte(m_flags);
}

void AddPlayerS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleAddPlayer(this);
}

int AddPlayerS2CPacket::GetEstimatedSize()
{
    return 36 + (int) m_username.size();
}

uint8_t AddPlayerS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

int AddPlayerS2CPacket::GetEntityId() const
{
    return m_entityId;
}

const std::string &AddPlayerS2CPacket::GetUsername() const
{
    return m_username;
}

double AddPlayerS2CPacket::GetX() const
{
    return m_x;
}

double AddPlayerS2CPacket::GetY() const
{
    return m_y;
}

double AddPlayerS2CPacket::GetZ() const
{
    return m_z;
}

float AddPlayerS2CPacket::GetYaw() const
{
    return m_yaw;
}

float AddPlayerS2CPacket::GetPitch() const
{
    return m_pitch;
}

uint8_t AddPlayerS2CPacket::GetFlags() const
{
    return m_flags;
}
