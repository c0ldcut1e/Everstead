#pragma once

#include <cstdint>
#include <vector>

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class MultiBlockUpdateS2CPacket : public Packet
{
public:
    struct Change
    {
        BlockPos pos;
        uint16_t blockId;
        uint16_t blockData;
    };

    MultiBlockUpdateS2CPacket();
    explicit MultiBlockUpdateS2CPacket(const std::vector<Change> &changes);
    ~MultiBlockUpdateS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::vector<Change> &GetChanges() const;

private:
    static constexpr uint32_t kMaxChanges = 65536;

    std::vector<Change> m_changes;
};
