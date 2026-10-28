#pragma once

#include <cstdint>

#include "../Packet.h"

class MoveEntityRotS2CPacket : public Packet
{
public:
    MoveEntityRotS2CPacket();
    MoveEntityRotS2CPacket(int entityId, uint8_t yaw, uint8_t pitch);
    ~MoveEntityRotS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    int GetEntityId() const;
    uint8_t GetYaw() const;
    uint8_t GetPitch() const;

private:
    int m_entityId;
    uint8_t m_yaw;
    uint8_t m_pitch;
};
