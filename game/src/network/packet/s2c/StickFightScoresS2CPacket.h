#pragma once

#include <array>

#include "../Packet.h"

class StickFightScoresS2CPacket : public Packet
{
public:
    StickFightScoresS2CPacket();
    StickFightScoresS2CPacket(const std::array<int, 4> &scores);
    ~StickFightScoresS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::array<int, 4> &GetScores() const;

private:
    std::array<int, 4> m_scores;
};
