#pragma once

#include "../Packet.h"

class SetExperienceS2CPacket : public Packet
{
public:
    SetExperienceS2CPacket();
    SetExperienceS2CPacket(float progress, int level, int total);
    ~SetExperienceS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    float GetProgress() const;
    int GetLevel() const;
    int GetTotal() const;

private:
    float m_progress;
    int m_level;
    int m_total;
};
