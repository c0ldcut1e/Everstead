#pragma once

#include <string>

#include "../Packet.h"

#include <glm/vec3.hpp>

class SoundC2SPacket : public Packet
{
public:
    SoundC2SPacket();
    SoundC2SPacket(const std::string &soundName, float volume, float pitch);
    SoundC2SPacket(const std::string &soundName, float volume, float pitch,
                           const glm::vec3 &position);
    ~SoundC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::string &GetSoundName() const;
    float GetVolume() const;
    float GetPitch() const;
    bool HasPosition() const;
    const glm::vec3 &GetPosition() const;

private:
    std::string m_soundName;
    float m_volume;
    float m_pitch;
    bool m_hasPosition;
    glm::vec3 m_position;
};
