#pragma once

#include "../Packet.h"

class AtmosphereC2SPacket : public Packet
{
public:
    AtmosphereC2SPacket();
    AtmosphereC2SPacket(float airDensity, float anisotropy);
    ~AtmosphereC2SPacket() override;

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
