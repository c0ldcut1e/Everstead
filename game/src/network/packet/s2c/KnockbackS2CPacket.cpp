#include "KnockbackS2CPacket.h"

#include "../../listener/PacketListener.h"

KnockbackS2CPacket::KnockbackS2CPacket()
{
    m_sourceX = 0.0f;
    m_sourceZ = 0.0f;
    m_pushX   = 0.0f;
    m_pushY   = 0.0f;
    m_pushZ   = 0.0f;
}

KnockbackS2CPacket::KnockbackS2CPacket(float sourceX, float sourceZ, const glm::dvec3 &push)
{
    m_sourceX = sourceX;
    m_sourceZ = sourceZ;
    m_pushX   = (float) push.x;
    m_pushY   = (float) push.y;
    m_pushZ   = (float) push.z;
}

KnockbackS2CPacket::~KnockbackS2CPacket()
{}

ePACKET_ID KnockbackS2CPacket::GetId()
{
    return ePacketId_KnockbackS2C;
}

void KnockbackS2CPacket::Read(DataInputStream *inputStream)
{
    m_sourceX = inputStream->ReadFloat();
    m_sourceZ = inputStream->ReadFloat();
    m_pushX   = inputStream->ReadFloat();
    m_pushY   = inputStream->ReadFloat();
    m_pushZ   = inputStream->ReadFloat();
}

void KnockbackS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteFloat(m_sourceX);
    outputStream->WriteFloat(m_sourceZ);
    outputStream->WriteFloat(m_pushX);
    outputStream->WriteFloat(m_pushY);
    outputStream->WriteFloat(m_pushZ);
}

void KnockbackS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleKnockback(this);
}

int KnockbackS2CPacket::GetEstimatedSize()
{
    return 21;
}

float KnockbackS2CPacket::GetSourceX() const
{
    return m_sourceX;
}

float KnockbackS2CPacket::GetSourceZ() const
{
    return m_sourceZ;
}

glm::dvec3 KnockbackS2CPacket::GetPush() const
{
    return glm::dvec3(m_pushX, m_pushY, m_pushZ);
}
