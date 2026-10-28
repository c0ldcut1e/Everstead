#pragma once

#include <cstdint>

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class BlockDestructionS2CPacket : public Packet
{
public:
    BlockDestructionS2CPacket();
    BlockDestructionS2CPacket(uint8_t playerIndex, const BlockPos &pos, int stage);
    ~BlockDestructionS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetPlayerIndex() const;
    const BlockPos &GetPos() const;
    int GetStage() const;

private:
    uint8_t m_playerIndex;
    BlockPos m_pos;
    int m_stage;
};
