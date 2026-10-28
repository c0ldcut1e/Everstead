#pragma once

#include <string>

#include "../Packet.h"

class StopSoundS2CPacket : public Packet
{
public:
    StopSoundS2CPacket();
    StopSoundS2CPacket(const std::string &source, const std::string &soundId);
    ~StopSoundS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::string &GetSource() const;
    const std::string &GetSoundId() const;

private:
    std::string m_source;
    std::string m_soundId;
};
