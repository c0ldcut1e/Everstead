#pragma once

#include "../../../world/WorldEffects.h"
#include "../Packet.h"

#include <glm/vec3.hpp>

class WorldEffectS2CPacket : public Packet
{
public:
    WorldEffectS2CPacket();
    WorldEffectS2CPacket(WorldEffects::eWORLD_EFFECT_TYPE type, const glm::dvec3 &center, float radius);
    ~WorldEffectS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    WorldEffects::eWORLD_EFFECT_TYPE GetType() const;
    const glm::dvec3 &GetCenter() const;
    float GetRadius() const;

private:
    WorldEffects::eWORLD_EFFECT_TYPE m_type;
    glm::dvec3 m_center;
    float m_radius;
};
