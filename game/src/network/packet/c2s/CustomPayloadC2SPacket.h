#pragma once

#include <cstdint>
#include <vector>

#include "../../../core/ResourceLocation.h"
#include "../Packet.h"

class CustomPayloadC2SPacket : public Packet
{
public:
    CustomPayloadC2SPacket();
    CustomPayloadC2SPacket(const ResourceLocation &channel, const std::vector<uint8_t> &payload);
    ~CustomPayloadC2SPacket() override;

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
