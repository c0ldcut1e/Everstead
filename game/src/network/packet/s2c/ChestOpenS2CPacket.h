#pragma once

#include <cstdint>

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class ChestOpenS2CPacket : public Packet
{
public:
    ChestOpenS2CPacket();
    ChestOpenS2CPacket(const BlockPos &pos, uint8_t openCount);
    ~ChestOpenS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const BlockPos &GetPos() const;
    uint8_t GetOpenCount() const;

private:
    BlockPos m_pos;
    uint8_t m_openCount;
};
