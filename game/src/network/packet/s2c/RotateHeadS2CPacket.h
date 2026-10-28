#pragma once

#include <cstdint>

#include "../Packet.h"

class RotateHeadS2CPacket : public Packet
{
public:
    RotateHeadS2CPacket();
    RotateHeadS2CPacket(int entityId, uint8_t headYaw);
    ~RotateHeadS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    int GetEntityId() const;
    uint8_t GetHeadYaw() const;

private:
    int m_entityId;
    uint8_t m_headYaw;
};
