#pragma once

#include <cstdint>

#include "../Packet.h"

class RemovePlayerS2CPacket : public Packet
{
public:
    RemovePlayerS2CPacket();
    explicit RemovePlayerS2CPacket(uint8_t playerIndex);
    ~RemovePlayerS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetPlayerIndex() const;

private:
    uint8_t m_playerIndex;
};
