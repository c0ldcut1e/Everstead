#pragma once

#include "../Packet.h"

class KeepAliveS2CPacket : public Packet
{
public:
    KeepAliveS2CPacket();
    ~KeepAliveS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
};
