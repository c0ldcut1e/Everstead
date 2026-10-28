#pragma once

#include <cstdint>

#include "../Packet.h"

class MovePlayerS2CPacket : public Packet
{
public:
    MovePlayerS2CPacket();
    MovePlayerS2CPacket(uint8_t playerIndex, double x, double y, double z, float yaw,
                                float pitch);
    ~MovePlayerS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    uint8_t GetPlayerIndex() const;
    double GetX() const;
    double GetY() const;
    double GetZ() const;
    float GetYaw() const;
    float GetPitch() const;

private:
    uint8_t m_playerIndex;
    double m_x;
    double m_y;
    double m_z;
    float m_yaw;
    float m_pitch;
};
