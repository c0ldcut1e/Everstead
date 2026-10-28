#pragma once

#include "../Packet.h"

class KeepAliveC2SPacket : public Packet
{
public:
    KeepAliveC2SPacket();
    ~KeepAliveC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
};
