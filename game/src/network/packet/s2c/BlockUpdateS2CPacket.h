#pragma once

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class BlockUpdateS2CPacket : public Packet
{
public:
    BlockUpdateS2CPacket();
    BlockUpdateS2CPacket(const BlockPos &pos, uint16_t blockId, uint16_t blockData);
    ~BlockUpdateS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const BlockPos &GetPos() const;
    uint16_t GetBlockId() const;
    uint16_t GetBlockData() const;

private:
    BlockPos m_pos;
    uint16_t m_blockId;
    uint16_t m_blockData;
};
