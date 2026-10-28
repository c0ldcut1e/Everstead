#pragma once

#include <cstdint>

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class PlayerSleepS2CPacket : public Packet
{
public:
    PlayerSleepS2CPacket();
    PlayerSleepS2CPacket(uint8_t playerIndex, bool sleeping, const BlockPos &pos);
    ~PlayerSleepS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetPlayerIndex() const;
    bool IsSleeping() const;
    const BlockPos &GetPos() const;

private:
    uint8_t m_playerIndex;
    bool m_sleeping;
    BlockPos m_pos;
};
