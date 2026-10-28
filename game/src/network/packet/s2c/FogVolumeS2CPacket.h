#pragma once

#include "../Packet.h"

class FogVolumeS2CPacket : public Packet
{
public:
    FogVolumeS2CPacket();
    explicit FogVolumeS2CPacket(const DynamicLights::FogVolume &value);
    explicit FogVolumeS2CPacket(uint32_t removedId);
    ~FogVolumeS2CPacket() override;

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
