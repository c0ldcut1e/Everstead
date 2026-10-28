#pragma once

#include <cstdint>

#include "../Packet.h"

class TeleportEntityS2CPacket : public Packet
{
public:
    TeleportEntityS2CPacket();
    TeleportEntityS2CPacket(int entityId, int64_t x, int64_t y, int64_t z, uint8_t yaw, uint8_t pitch, bool onGround);
    ~TeleportEntityS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    int GetEntityId() const;
    int64_t GetX() const;
    int64_t GetY() const;
    int64_t GetZ() const;
    uint8_t GetYaw() const;
    uint8_t GetPitch() const;
    bool IsOnGround() const;

private:
    int m_entityId;
    int64_t m_x;
    int64_t m_y;
    int64_t m_z;
    uint8_t m_yaw;
    uint8_t m_pitch;
    bool m_onGround;
};
