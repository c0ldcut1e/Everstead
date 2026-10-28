#pragma once

#include <cstdint>
#include <vector>

#include "../../../core/ResourceLocation.h"
#include "../Packet.h"

class CustomPayloadS2CPacket : public Packet
{
public:
    CustomPayloadS2CPacket();
    CustomPayloadS2CPacket(const ResourceLocation &channel, const std::vector<uint8_t> &payload);
    ~CustomPayloadS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const ResourceLocation &GetChannel() const;
    const std::vector<uint8_t> &GetPayload() const;

    static const int s_maxPayloadSize;

private:
    ResourceLocation m_channel;
    std::vector<uint8_t> m_payload;
};
