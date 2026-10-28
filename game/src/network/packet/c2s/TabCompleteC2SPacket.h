#pragma once

#include <string>

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class TabCompleteC2SPacket : public Packet
{
public:
    TabCompleteC2SPacket();
    TabCompleteC2SPacket(const std::string &text, bool hasTarget, const BlockPos &target);
    ~TabCompleteC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::string &GetText() const;
    bool HasTarget() const;
    const BlockPos &GetTarget() const;

private:
    std::string m_text;
    bool m_hasTarget;
    BlockPos m_target;
};
