#pragma once

#include "../Packet.h"

class SwingC2SPacket : public Packet
{
public:
    SwingC2SPacket();
    ~SwingC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;
};
