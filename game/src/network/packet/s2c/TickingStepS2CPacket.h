#pragma once

#include "../Packet.h"

class TickingStepS2CPacket : public Packet
{
public:
    TickingStepS2CPacket();
    TickingStepS2CPacket(int steps);
    ~TickingStepS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetSteps() const;

private:
    int m_steps;
};
