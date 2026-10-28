#include "SoundC2SPacket.h"

#include "../../listener/PacketListener.h"

SoundC2SPacket::SoundC2SPacket()
{
    m_soundName   = "";
    m_volume      = 1.0f;
    m_pitch       = 1.0f;
    m_hasPosition = false;
    m_position    = glm::vec3(0.0f, 0.0f, 0.0f);
}

SoundC2SPacket::SoundC2SPacket(const std::string &soundName, float volume, float pitch)
{
    m_soundName   = soundName;
    m_volume      = volume;
    m_pitch       = pitch;
    m_hasPosition = false;
    m_position    = glm::vec3(0.0f, 0.0f, 0.0f);
}

SoundC2SPacket::SoundC2SPacket(const std::string &soundName, float volume, float pitch, const glm::vec3 &position)
{
    m_soundName   = soundName;
    m_volume      = volume;
    m_pitch       = pitch;
    m_hasPosition = true;
    m_position    = position;
}

SoundC2SPacket::~SoundC2SPacket()
{}

ePACKET_ID SoundC2SPacket::GetId()
{
    return ePacketId_SoundC2S;
}

void SoundC2SPacket::Read(DataInputStream *inputStream)
{
    m_soundName   = inputStream->ReadString();
    m_volume      = inputStream->ReadFloat();
    m_pitch       = inputStream->ReadFloat();
    m_hasPosition = inputStream->ReadByte() != 0;
    // Separate statements keep the read order fixed, arguments of one call are evaluated in any order
    float x    = inputStream->ReadFloat();
    float y    = inputStream->ReadFloat();
    float z    = inputStream->ReadFloat();
    m_position = glm::vec3(x, y, z);
}

void SoundC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_soundName);
    outputStream->WriteFloat(m_volume);
    outputStream->WriteFloat(m_pitch);
    outputStream->WriteByte(m_hasPosition ? 1 : 0);
    outputStream->WriteFloat(m_position.x);
    outputStream->WriteFloat(m_position.y);
    outputStream->WriteFloat(m_position.z);
}

void SoundC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleSound(this);
}

int SoundC2SPacket::GetEstimatedSize()
{
    return 21 + (int) m_soundName.size();
}

const std::string &SoundC2SPacket::GetSoundName() const
{
    return m_soundName;
}

float SoundC2SPacket::GetVolume() const
{
    return m_volume;
}

float SoundC2SPacket::GetPitch() const
{
    return m_pitch;
}

bool SoundC2SPacket::HasPosition() const
{
    return m_hasPosition;
}

const glm::vec3 &SoundC2SPacket::GetPosition() const
{
    return m_position;
}
