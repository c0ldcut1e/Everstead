#pragma once

#include <memory>

#include "../../../tag/CompoundTag.h"
#include "../Packet.h"

class TeamsS2CPacket : public Packet
{
public:
    TeamsS2CPacket();
    TeamsS2CPacket(std::unique_ptr<CompoundTag> tag);
    ~TeamsS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const CompoundTag *GetTag() const;

private:
    std::unique_ptr<CompoundTag> m_tag;
};
