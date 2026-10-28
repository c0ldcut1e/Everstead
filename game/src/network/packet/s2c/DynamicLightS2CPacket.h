#pragma once

#include "../Packet.h"

class DynamicLightS2CPacket : public Packet
{
public:
    DynamicLightS2CPacket();
    explicit DynamicLightS2CPacket(const DynamicLights::Light &value);
    explicit DynamicLightS2CPacket(uint32_t removedId);
    ~DynamicLightS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    bool IsRemoved() const;
    const DynamicLights::Light &GetLight() const;

private:
    DynamicLights::Light m_value;
    bool m_removed;
};
