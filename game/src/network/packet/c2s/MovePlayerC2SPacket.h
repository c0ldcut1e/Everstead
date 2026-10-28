#pragma once

#include "../Packet.h"

class MovePlayerC2SPacket : public Packet
{
public:
    MovePlayerC2SPacket();
    MovePlayerC2SPacket(double x, double y, double z, float yaw, float pitch, bool onGround, bool flying, uint32_t teleportId);
    ~MovePlayerC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    double GetX() const;
    double GetY() const;
    double GetZ() const;
    float GetYaw() const;
    float GetPitch() const;
    bool IsOnGround() const;
    bool IsFlying() const;
    uint32_t GetTeleportId() const;

private:
    double m_x;
    double m_y;
    double m_z;
    float m_yaw;
    float m_pitch;
    bool m_onGround;
    bool m_flying;
    uint32_t m_teleportId;
};
