#pragma once

#include <string>

#include "../Packet.h"

#include <glm/vec3.hpp>

class SoundS2CPacket : public Packet
{
public:
    SoundS2CPacket();
    SoundS2CPacket(const std::string &soundName, float volume, float pitch,
                           const glm::vec3 &position);
    ~SoundS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::string &GetSoundName() const;
    float GetVolume() const;
    float GetPitch() const;
    const glm::vec3 &GetPosition() const;

private:
    std::string m_soundName;
    float m_volume;
    float m_pitch;
    glm::vec3 m_position;
};
