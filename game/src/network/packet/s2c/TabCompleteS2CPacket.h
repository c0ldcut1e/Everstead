#pragma once

#include <string>
#include <vector>

#include "../Packet.h"

class TabCompleteS2CPacket : public Packet
{
public:
    TabCompleteS2CPacket();
    explicit TabCompleteS2CPacket(const std::vector<std::string> &completions);
    ~TabCompleteS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::vector<std::string> &GetCompletions() const;

private:
    static constexpr uint16_t kMaxCompletions = 1024;

    std::vector<std::string> m_completions;
};
