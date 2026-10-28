#include "SetExperienceS2CPacket.h"

#include "../../listener/PacketListener.h"

SetExperienceS2CPacket::SetExperienceS2CPacket()
{
    m_progress = 0.0f;
    m_level    = 0;
    m_total    = 0;
}

SetExperienceS2CPacket::SetExperienceS2CPacket(float progress, int level, int total)
{
    m_progress = progress;
    m_level    = level;
    m_total    = total;
}

SetExperienceS2CPacket::~SetExperienceS2CPacket()
{}

ePACKET_ID SetExperienceS2CPacket::GetId()
{
    return ePacketId_SetExperienceS2C;
}

void SetExperienceS2CPacket::Read(DataInputStream *inputStream)
{
    m_progress = inputStream->ReadFloat();
    m_level    = (int) inputStream->ReadInt();
    m_total    = (int) inputStream->ReadInt();
}

void SetExperienceS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteFloat(m_progress);
    outputStream->WriteInt((uint32_t) m_level);
    outputStream->WriteInt((uint32_t) m_total);
}

void SetExperienceS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetExperience(this);
}

int SetExperienceS2CPacket::GetEstimatedSize()
{
    return 12;
}

float SetExperienceS2CPacket::GetProgress() const
{
    return m_progress;
}

int SetExperienceS2CPacket::GetLevel() const
{
    return m_level;
}

int SetExperienceS2CPacket::GetTotal() const
{
    return m_total;
}
