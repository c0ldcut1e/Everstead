#pragma once

#include <vector>

#include "../../../world/Explosion.h"
#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

#include <glm/vec3.hpp>

class ExplodeS2CPacket : public Packet
{
public:
    ExplodeS2CPacket();
    ExplodeS2CPacket(const glm::dvec3 &center, float radius, const std::vector<BlockPos> &blocks, const glm::dvec3 &knockback, eEXPLOSION_TYPE type);
    ~ExplodeS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const glm::dvec3 &GetCenter() const;
    float GetRadius() const;
    const std::vector<BlockPos> &GetBlocks() const;
    const glm::dvec3 &GetKnockback() const;
    eEXPLOSION_TYPE GetType() const;

private:
    glm::dvec3 m_center;
    float m_radius;
    std::vector<BlockPos> m_blocks;
    glm::dvec3 m_knockback;
    eEXPLOSION_TYPE m_type;
};
