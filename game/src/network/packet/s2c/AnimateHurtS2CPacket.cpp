#include "AnimateHurtS2CPacket.h"

#include "../../listener/PacketListener.h"

AnimateHurtS2CPacket::AnimateHurtS2CPacket()
{
    m_entityId = 0;
}

AnimateHurtS2CPacket::AnimateHurtS2CPacket(int entityId)
{
    m_entityId = entityId;
}

AnimateHurtS2CPacket::~AnimateHurtS2CPacket()
{}

ePACKET_ID AnimateHurtS2CPacket::GetId()
{
    return ePacketId_AnimateHurtS2C;
}

void AnimateHurtS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
}

void AnimateHurtS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
}

void AnimateHurtS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleAnimateHurt(this);
}

int AnimateHurtS2CPacket::GetEstimatedSize()
{
    return 5;
}

int AnimateHurtS2CPacket::GetEntityId() const
{
    return m_entityId;
}
