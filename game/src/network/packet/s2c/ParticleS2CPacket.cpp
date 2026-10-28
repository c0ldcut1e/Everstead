#include "ParticleS2CPacket.h"

#include "../../listener/PacketListener.h"

ParticleS2CPacket::ParticleS2CPacket()
{
    m_type     = eParticleType_NotSet;
    m_position = glm::dvec3();
    m_delta    = glm::dvec3();
    m_speed    = 0.0f;
    m_count    = 0;
}

ParticleS2CPacket::ParticleS2CPacket(ePARTICLE_TYPE type, const glm::dvec3 &position, const glm::dvec3 &delta, float speed, int count)
{
    m_type     = type;
    m_position = position;
    m_delta    = delta;
    m_speed    = speed;
    m_count    = count;
}

ParticleS2CPacket::~ParticleS2CPacket()
{}

ePACKET_ID ParticleS2CPacket::GetId()
{
    return ePacketId_ParticleS2C;
}

void ParticleS2CPacket::Read(DataInputStream *inputStream)
{
    m_type     = (ePARTICLE_TYPE) inputStream->ReadShort();
    double x   = inputStream->ReadDouble();
    double y   = inputStream->ReadDouble();
    double z   = inputStream->ReadDouble();
    float dx   = inputStream->ReadFloat();
    float dy   = inputStream->ReadFloat();
    float dz   = inputStream->ReadFloat();
    m_position = glm::dvec3(x, y, z);
    m_delta    = glm::dvec3(dx, dy, dz);
    m_speed    = inputStream->ReadFloat();
    m_count    = (int) inputStream->ReadInt();
}

void ParticleS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteShort(m_type);
    outputStream->WriteDouble(m_position.x);
    outputStream->WriteDouble(m_position.y);
    outputStream->WriteDouble(m_position.z);
    outputStream->WriteFloat((float) m_delta.x);
    outputStream->WriteFloat((float) m_delta.y);
    outputStream->WriteFloat((float) m_delta.z);
    outputStream->WriteFloat(m_speed);
    outputStream->WriteInt((uint32_t) m_count);
}

void ParticleS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleParticle(this);
}

int ParticleS2CPacket::GetEstimatedSize()
{
    return 46;
}

ePARTICLE_TYPE ParticleS2CPacket::GetType() const
{
    return m_type;
}

const glm::dvec3 &ParticleS2CPacket::GetPosition() const
{
    return m_position;
}

const glm::dvec3 &ParticleS2CPacket::GetDelta() const
{
    return m_delta;
}

float ParticleS2CPacket::GetSpeed() const
{
    return m_speed;
}

int ParticleS2CPacket::GetCount() const
{
    return m_count;
}
