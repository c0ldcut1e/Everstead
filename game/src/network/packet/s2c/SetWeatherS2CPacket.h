#pragma once

#include "../Packet.h"

class SetWeatherS2CPacket : public Packet
{
public:
    SetWeatherS2CPacket();
    SetWeatherS2CPacket(bool raining, bool thundering, float rainLevel, float thunderLevel);
    ~SetWeatherS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    bool IsRaining() const;
    bool IsThundering() const;
    float GetRainLevel() const;
    float GetThunderLevel() const;

private:
    bool m_raining;
    bool m_thundering;
    float m_rainLevel;
    float m_thunderLevel;
};
