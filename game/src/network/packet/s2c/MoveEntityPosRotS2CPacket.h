#pragma once

#include <cstdint>

#include "../Packet.h"

class MoveEntityPosRotS2CPacket : public Packet
{
public:
    MoveEntityPosRotS2CPacket();
    MoveEntityPosRotS2CPacket(int entityId, int16_t xa, int16_t ya, int16_t za, uint8_t yaw, uint8_t pitch, bool onGround);
    ~MoveEntityPosRotS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    int GetEntityId() const;
    int16_t GetXa() const;
    int16_t GetYa() const;
    int16_t GetZa() const;
    int16_t GetYaw() const;
    uint8_t GetPitch() const;
    bool IsOnGround() const;

private:
    int m_entityId;
    int16_t m_xa;
    int16_t m_ya;
    int16_t m_za;
    int16_t m_yaw;
    uint8_t m_pitch;
    bool m_onGround;
};
