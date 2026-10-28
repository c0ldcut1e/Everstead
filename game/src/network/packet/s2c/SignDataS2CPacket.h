#pragma once

#include <memory>

#include "../../../tag/CompoundTag.h"
#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class SignDataS2CPacket : public Packet
{
public:
    SignDataS2CPacket();
    SignDataS2CPacket(const BlockPos &pos, std::unique_ptr<CompoundTag> tag);
    ~SignDataS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const BlockPos &GetPos() const;
    const CompoundTag *GetTag() const;

private:
    BlockPos m_pos;
    std::unique_ptr<CompoundTag> m_tag;
};
