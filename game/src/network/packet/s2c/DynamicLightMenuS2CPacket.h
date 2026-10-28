#pragma once

#include "../Packet.h"

class DynamicLightMenuS2CPacket : public Packet
{
public:
    DynamicLightMenuS2CPacket();
    ~DynamicLightMenuS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;
};
