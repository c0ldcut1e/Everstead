#include "StopSoundS2CPacket.h"

#include "../../listener/PacketListener.h"

StopSoundS2CPacket::StopSoundS2CPacket()
{
    m_source  = "";
    m_soundId = "";
}

StopSoundS2CPacket::StopSoundS2CPacket(const std::string &source, const std::string &soundId)
{
    m_source  = source;
    m_soundId = soundId;
}

StopSoundS2CPacket::~StopSoundS2CPacket()
{}

ePACKET_ID StopSoundS2CPacket::GetId()
{
    return ePacketId_StopSoundS2C;
}

void StopSoundS2CPacket::Read(DataInputStream *inputStream)
{
    m_source  = inputStream->ReadString();
    m_soundId = inputStream->ReadString();
}

void StopSoundS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_source);
    outputStream->WriteString(m_soundId);
}

void StopSoundS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleStopSound(this);
}

int StopSoundS2CPacket::GetEstimatedSize()
{
    return 4 + (int) m_source.size() + (int) m_soundId.size();
}

const std::string &StopSoundS2CPacket::GetSource() const
{
    return m_source;
}

const std::string &StopSoundS2CPacket::GetSoundId() const
{
    return m_soundId;
}
