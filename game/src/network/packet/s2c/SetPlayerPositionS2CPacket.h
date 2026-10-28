#pragma once

#include "../Packet.h"

#include <glm/vec3.hpp>

class SetPlayerPositionS2CPacket : public Packet
{
public:
    SetPlayerPositionS2CPacket();
    SetPlayerPositionS2CPacket(double x, double y, double z, float yaw, float pitch, uint32_t teleportId, const glm::dvec3 &velocity = glm::dvec3());
    ~SetPlayerPositionS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    double GetX() const;
    double GetY() const;
    double GetZ() const;
    float GetYaw() const;
    float GetPitch() const;
    uint32_t GetTeleportId() const;
    glm::dvec3 GetVelocity() const;

private:
    double m_x;
    double m_y;
    double m_z;
    float m_yaw;
    float m_pitch;
    uint32_t m_teleportId;
    float m_velocityX;
    float m_velocityY;
    float m_velocityZ;
};
