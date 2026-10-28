#pragma once

#include "../Packet.h"

class RespawnC2SPacket : public Packet
{
public:
    RespawnC2SPacket();
    ~RespawnC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;
};
