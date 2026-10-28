#pragma once

#include <cstdint>

#include "../Packet.h"

class SetTimeS2CPacket : public Packet
{
public:
    SetTimeS2CPacket();
    SetTimeS2CPacket(int64_t ticks, bool daylightCycle);
    ~SetTimeS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int64_t GetTicks() const;
    bool IsDaylightCycle() const;

private:
    int64_t m_ticks;
    bool m_daylightCycle;
};
