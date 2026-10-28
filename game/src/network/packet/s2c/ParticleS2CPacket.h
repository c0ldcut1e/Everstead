#pragma once

#include <cstdint>

#include "../../../world/particle/ParticleType.h"
#include "../Packet.h"

#include <glm/vec3.hpp>

class ParticleS2CPacket : public Packet
{
public:
    ParticleS2CPacket();
    ParticleS2CPacket(ePARTICLE_TYPE type, const glm::dvec3 &position, const glm::dvec3 &delta, float speed, int count);
    ~ParticleS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    ePARTICLE_TYPE GetType() const;
    const glm::dvec3 &GetPosition() const;
    const glm::dvec3 &GetDelta() const;
    float GetSpeed() const;
    int GetCount() const;

private:
    ePARTICLE_TYPE m_type;
    glm::dvec3 m_position;
    glm::dvec3 m_delta;
    float m_speed;
    int m_count;
};
