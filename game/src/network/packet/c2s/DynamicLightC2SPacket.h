#pragma once

#include "../Packet.h"

class DynamicLightC2SPacket : public Packet
{
public:
    DynamicLightC2SPacket();
    explicit DynamicLightC2SPacket(const DynamicLights::Light &value);
    explicit DynamicLightC2SPacket(uint32_t removedId);
    ~DynamicLightC2SPacket() override;

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
