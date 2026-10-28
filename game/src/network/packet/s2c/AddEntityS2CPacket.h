#pragma once

#include <cstdint>

#include "../../../entity/EntityType.h"
#include "../Packet.h"

#include <glm/vec3.hpp>

class AddEntityS2CPacket : public Packet
{
public:
    AddEntityS2CPacket();
    AddEntityS2CPacket(int entityId, eENTITY_TYPE type, const glm::dvec3 &position, const glm::dvec3 &velocity, float yaw, float pitch, int data);
    ~AddEntityS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;
    eENTITY_TYPE GetType() const;
    const glm::dvec3 &GetPosition() const;
    const glm::dvec3 &GetVelocity() const;
    float GetYaw() const;
    float GetPitch() const;
    int GetData() const;
    bool IsValid() const;

private:
    int m_entityId;
    eENTITY_TYPE m_type;
    glm::dvec3 m_position;
    glm::dvec3 m_velocity;
    float m_yaw;
    float m_pitch;
    int m_data;
};
