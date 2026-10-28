#pragma once

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class PickBlockC2SPacket : public Packet
{
public:
    PickBlockC2SPacket();
    PickBlockC2SPacket(const BlockPos &pos, uint8_t slot);
    ~PickBlockC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const BlockPos &GetPos() const;
    uint8_t GetSlot() const;

private:
    BlockPos m_pos;
    uint8_t m_slot;
};
