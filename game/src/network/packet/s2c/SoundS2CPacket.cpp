#include "SoundS2CPacket.h"

#include "../../listener/PacketListener.h"

SoundS2CPacket::SoundS2CPacket()
{
    m_soundName = "";
    m_volume    = 1.0f;
    m_pitch     = 1.0f;
    m_position  = glm::vec3(0.0f, 0.0f, 0.0f);
}

SoundS2CPacket::SoundS2CPacket(const std::string &soundName, float volume, float pitch, const glm::vec3 &position)
{
    m_soundName = soundName;
    m_volume    = volume;
    m_pitch     = pitch;
    m_position  = position;
}

SoundS2CPacket::~SoundS2CPacket()
{}

ePACKET_ID SoundS2CPacket::GetId()
{
    return ePacketId_SoundS2C;
}

void SoundS2CPacket::Read(DataInputStream *inputStream)
{
    m_soundName = inputStream->ReadString();
    m_volume    = inputStream->ReadFloat();
    m_pitch     = inputStream->ReadFloat();
    // Separate statements keep the read order fixed, arguments of one call are evaluated in any order
    float x    = inputStream->ReadFloat();
    float y    = inputStream->ReadFloat();
    float z    = inputStream->ReadFloat();
    m_position = glm::vec3(x, y, z);
}

void SoundS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_soundName);
    outputStream->WriteFloat(m_volume);
    outputStream->WriteFloat(m_pitch);
    outputStream->WriteFloat(m_position.x);
    outputStream->WriteFloat(m_position.y);
    outputStream->WriteFloat(m_position.z);
}

void SoundS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSound(this);
}

int SoundS2CPacket::GetEstimatedSize()
{
    return 18 + (int) m_soundName.size();
}

const std::string &SoundS2CPacket::GetSoundName() const
{
    return m_soundName;
}

float SoundS2CPacket::GetVolume() const
{
    return m_volume;
}

float SoundS2CPacket::GetPitch() const
{
    return m_pitch;
}

const glm::vec3 &SoundS2CPacket::GetPosition() const
{
    return m_position;
}
