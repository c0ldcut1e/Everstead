#pragma once

#include <cstdint>

#include "../Packet.h"

#include <glm/vec3.hpp>

class SetEntityMotionS2CPacket : public Packet
{
public:
    SetEntityMotionS2CPacket();
    SetEntityMotionS2CPacket(int entityId, const glm::dvec3 &velocity);
    ~SetEntityMotionS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    int GetEntityId() const;
    glm::dvec3 GetVelocity() const;

private:
    int m_entityId;
    int16_t m_xa;
    int16_t m_ya;
    int16_t m_za;
};
