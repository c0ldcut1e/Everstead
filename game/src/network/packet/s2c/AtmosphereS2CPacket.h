#pragma once

#include "../Packet.h"

class AtmosphereS2CPacket : public Packet
{
public:
    AtmosphereS2CPacket();
    AtmosphereS2CPacket(float airDensity, float anisotropy);
    ~AtmosphereS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    float GetAirDensity() const;
    float GetAnisotropy() const;

private:
    float m_airDensity;
    float m_anisotropy;
};
