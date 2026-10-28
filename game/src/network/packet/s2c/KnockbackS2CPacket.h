#pragma once

#include "../Packet.h"

#include <glm/vec3.hpp>

class KnockbackS2CPacket : public Packet
{
public:
    KnockbackS2CPacket();
    KnockbackS2CPacket(float sourceX, float sourceZ, const glm::dvec3 &push);
    ~KnockbackS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    float GetSourceX() const;
    float GetSourceZ() const;
    glm::dvec3 GetPush() const;

private:
    float m_sourceX;
    float m_sourceZ;
    float m_pushX;
    float m_pushY;
    float m_pushZ;
};
