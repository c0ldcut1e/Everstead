#include "AddEntityS2CPacket.h"

#include <cmath>

#include "../../listener/PacketListener.h"

AddEntityS2CPacket::AddEntityS2CPacket()
{
    m_entityId = 0;
    m_type     = eEntityType_Entity;
    m_position = glm::dvec3(0.0, 0.0, 0.0);
    m_velocity = glm::dvec3(0.0, 0.0, 0.0);
    m_yaw      = 0.0f;
    m_pitch    = 0.0f;
    m_data     = 0;
}

AddEntityS2CPacket::AddEntityS2CPacket(int entityId, eENTITY_TYPE type, const glm::dvec3 &position, const glm::dvec3 &velocity, float yaw, float pitch, int data)
{
    m_entityId = entityId;
    m_type     = type;
    m_position = position;
    m_velocity = velocity;
    m_yaw      = yaw;
    m_pitch    = pitch;
    m_data     = data;
}

AddEntityS2CPacket::~AddEntityS2CPacket()
{}

ePACKET_ID AddEntityS2CPacket::GetId()
{
    return ePacketId_AddEntityS2C;
}

void AddEntityS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId   = (int) inputStream->ReadInt();
    m_type       = (eENTITY_TYPE) inputStream->ReadShort();
    m_position.x = inputStream->ReadDouble();
    m_position.y = inputStream->ReadDouble();
    m_position.z = inputStream->ReadDouble();
    m_velocity.x = inputStream->ReadDouble();
    m_velocity.y = inputStream->ReadDouble();
    m_velocity.z = inputStream->ReadDouble();
    m_yaw        = inputStream->ReadFloat();
    m_pitch      = inputStream->ReadFloat();
    m_data       = (int) inputStream->ReadInt();
}

void AddEntityS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteShort(m_type);
    outputStream->WriteDouble(m_position.x);
    outputStream->WriteDouble(m_position.y);
    outputStream->WriteDouble(m_position.z);
    outputStream->WriteDouble(m_velocity.x);
    outputStream->WriteDouble(m_velocity.y);
    outputStream->WriteDouble(m_velocity.z);
    outputStream->WriteFloat(m_yaw);
    outputStream->WriteFloat(m_pitch);
    outputStream->WriteInt((uint32_t) m_data);
}

void AddEntityS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleAddEntity(this);
}

int AddEntityS2CPacket::GetEstimatedSize()
{
    return 62;
}

int AddEntityS2CPacket::GetEntityId() const
{
    return m_entityId;
}

eENTITY_TYPE AddEntityS2CPacket::GetType() const
{
    return m_type;
}

const glm::dvec3 &AddEntityS2CPacket::GetPosition() const
{
    return m_position;
}

const glm::dvec3 &AddEntityS2CPacket::GetVelocity() const
{
    return m_velocity;
}

float AddEntityS2CPacket::GetYaw() const
{
    return m_yaw;
}

float AddEntityS2CPacket::GetPitch() const
{
    return m_pitch;
}

int AddEntityS2CPacket::GetData() const
{
    return m_data;
}

bool AddEntityS2CPacket::IsValid() const
{
    return m_entityId > 0 && std::isfinite(m_position.x) && std::isfinite(m_position.y) && std::isfinite(m_position.z) &&
           std::isfinite(m_velocity.x) && std::isfinite(m_velocity.y) && std::isfinite(m_velocity.z) && std::isfinite(m_yaw) &&
           std::isfinite(m_pitch);
}
