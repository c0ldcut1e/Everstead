#pragma once

#include <cstdint>

#include "../../../world/WorldBorder.h"
#include "../Packet.h"

class WorldBorderS2CPacket : public Packet
{
public:
    WorldBorderS2CPacket();
    WorldBorderS2CPacket(const WorldBorder &border);
    ~WorldBorderS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    void Apply(WorldBorder *border) const;

private:
    double m_centerX;
    double m_centerZ;
    double m_size;
    double m_lerpTarget;
    int64_t m_lerpTicks;
    int m_warningBlocks;
    int m_warningTime;
};
