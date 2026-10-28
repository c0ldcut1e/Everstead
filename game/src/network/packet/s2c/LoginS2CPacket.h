#pragma once

#include "../Packet.h"

class LoginS2CPacket : public Packet
{
public:
    LoginS2CPacket();
    ~LoginS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
};
