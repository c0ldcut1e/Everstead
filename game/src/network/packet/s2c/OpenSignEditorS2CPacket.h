#pragma once

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class OpenSignEditorS2CPacket : public Packet
{
public:
    OpenSignEditorS2CPacket();
    OpenSignEditorS2CPacket(const BlockPos &pos, bool front);
    ~OpenSignEditorS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const BlockPos &GetPos() const;
    bool IsFront() const;

private:
    BlockPos m_pos;
    bool m_front;
};
