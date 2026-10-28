#pragma once

#include <string>

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class SignUpdateC2SPacket : public Packet
{
public:
    SignUpdateC2SPacket();
    SignUpdateC2SPacket(const BlockPos &pos, bool front, const std::string *lines);
    ~SignUpdateC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const BlockPos &GetPos() const;
    bool IsFront() const;
    const std::string &GetLine(int line) const;

private:
    BlockPos m_pos;
    bool m_front;
    std::string m_lines[4];
};
