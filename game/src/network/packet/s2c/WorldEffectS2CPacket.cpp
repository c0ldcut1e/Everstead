#include "WorldEffectS2CPacket.h"

#include <algorithm>

#include "../../listener/PacketListener.h"

WorldEffectS2CPacket::WorldEffectS2CPacket()
{
    m_type   = WorldEffects::eWorldEffectType_RiftShockwave;
    m_center = glm::dvec3(0.0);
    m_radius = 0.0f;
}

WorldEffectS2CPacket::WorldEffectS2CPacket(WorldEffects::eWORLD_EFFECT_TYPE type, const glm::dvec3 &center, float radius)
{
    m_type   = type;
    m_center = center;
    m_radius = radius;
}

WorldEffectS2CPacket::~WorldEffectS2CPacket()
{
}

ePACKET_ID WorldEffectS2CPacket::GetId()
{
    return ePacketId_WorldEffectS2C;
}

void WorldEffectS2CPacket::Read(DataInputStream *inputStream)
{
    m_type     = (WorldEffects::eWORLD_EFFECT_TYPE) std::min((int) inputStream->ReadByte(), (int) WorldEffects::eWorldEffectType_Count - 1);
    m_center.x = inputStream->ReadDouble();
    m_center.y = inputStream->ReadDouble();
    m_center.z = inputStream->ReadDouble();
    m_radius   = inputStream->ReadFloat();
}

void WorldEffectS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_type);
    outputStream->WriteDouble(m_center.x);
    outputStream->WriteDouble(m_center.y);
    outputStream->WriteDouble(m_center.z);
    outputStream->WriteFloat(m_radius);
}

void WorldEffectS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleWorldEffect(this);
}

int WorldEffectS2CPacket::GetEstimatedSize()
{
    return 29;
}

WorldEffects::eWORLD_EFFECT_TYPE WorldEffectS2CPacket::GetType() const
{
    return m_type;
}

const glm::dvec3 &WorldEffectS2CPacket::GetCenter() const
{
    return m_center;
}

float WorldEffectS2CPacket::GetRadius() const
{
    return m_radius;
}
