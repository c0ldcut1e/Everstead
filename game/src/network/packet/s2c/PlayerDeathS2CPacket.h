#pragma once

#include <cstdint>
#include <string>

#include "../Packet.h"

class PlayerDeathS2CPacket : public Packet
{
public:
    PlayerDeathS2CPacket();
    PlayerDeathS2CPacket(uint8_t playerIndex, const std::string &message);
    ~PlayerDeathS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetPlayerIndex() const;
    const std::string &GetMessage() const;

private:
    uint8_t m_playerIndex;
    std::string m_message;
};
