#include "SetEntityMotionS2CPacket.h"

#include <algorithm>

#include "../../listener/PacketListener.h"

SetEntityMotionS2CPacket::SetEntityMotionS2CPacket()
{
    m_entityId = 0;
    m_xa       = 0;
    m_ya       = 0;
    m_za       = 0;
}

SetEntityMotionS2CPacket::SetEntityMotionS2CPacket(int entityId, const glm::dvec3 &velocity)
{
    m_entityId = entityId;
    m_xa       = (int16_t) (std::clamp(velocity.x, -3.9, 3.9) * 8000.0);
    m_ya       = (int16_t) (std::clamp(velocity.y, -3.9, 3.9) * 8000.0);
    m_za       = (int16_t) (std::clamp(velocity.z, -3.9, 3.9) * 8000.0);
}

SetEntityMotionS2CPacket::~SetEntityMotionS2CPacket()
{}

ePACKET_ID SetEntityMotionS2CPacket::GetId()
{
    return ePacketId_SetEntityMotionS2C;
}

void SetEntityMotionS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    m_xa       = (int16_t) inputStream->ReadShort();
    m_ya       = (int16_t) inputStream->ReadShort();
    m_za       = (int16_t) inputStream->ReadShort();
}

void SetEntityMotionS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteShort((uint16_t) m_xa);
    outputStream->WriteShort((uint16_t) m_ya);
    outputStream->WriteShort((uint16_t) m_za);
}

void SetEntityMotionS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetEntityMotion(this);
}

int SetEntityMotionS2CPacket::GetEntityId() const
{
    return m_entityId;
}

glm::dvec3 SetEntityMotionS2CPacket::GetVelocity() const
{
    return glm::dvec3(m_xa, m_ya, m_za) / 8000.0;
}
