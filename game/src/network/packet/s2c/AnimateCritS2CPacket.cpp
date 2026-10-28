#include "AnimateCritS2CPacket.h"

#include "../../listener/PacketListener.h"

AnimateCritS2CPacket::AnimateCritS2CPacket()
{
    m_entityId = 0;
}

AnimateCritS2CPacket::AnimateCritS2CPacket(int entityId)
{
    m_entityId = entityId;
}

AnimateCritS2CPacket::~AnimateCritS2CPacket()
{}

ePACKET_ID AnimateCritS2CPacket::GetId()
{
    return ePacketId_AnimateCritS2C;
}

void AnimateCritS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
}

void AnimateCritS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
}

void AnimateCritS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleAnimateCrit(this);
}

int AnimateCritS2CPacket::GetEstimatedSize()
{
    return 5;
}

int AnimateCritS2CPacket::GetEntityId() const
{
    return m_entityId;
}
