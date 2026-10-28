#pragma once

#include "../Packet.h"

class FogVolumeC2SPacket : public Packet
{
public:
    FogVolumeC2SPacket();
    explicit FogVolumeC2SPacket(const DynamicLights::FogVolume &value);
    explicit FogVolumeC2SPacket(uint32_t removedId);
    ~FogVolumeC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    bool IsRemoved() const;
    const DynamicLights::FogVolume &GetFogVolume() const;

private:
    DynamicLights::FogVolume m_value;
    bool m_removed;
};
