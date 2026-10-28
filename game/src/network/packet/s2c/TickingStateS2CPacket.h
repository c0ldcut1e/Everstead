#pragma once

#include "../Packet.h"

class TickingStateS2CPacket : public Packet
{
public:
    TickingStateS2CPacket();
    TickingStateS2CPacket(float tickRate, bool frozen);
    ~TickingStateS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    float GetTickRate() const;
    bool IsFrozen() const;

private:
    float m_tickRate;
    bool m_frozen;
};
