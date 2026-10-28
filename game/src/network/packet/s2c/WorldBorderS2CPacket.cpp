#include "WorldBorderS2CPacket.h"

#include <algorithm>
#include <cmath>

#include "../../listener/PacketListener.h"

WorldBorderS2CPacket::WorldBorderS2CPacket()
{
    m_centerX       = 0.0;
    m_centerZ       = 0.0;
    m_size          = 0.0;
    m_lerpTarget    = 0.0;
    m_lerpTicks     = 0;
    m_warningBlocks = 0;
    m_warningTime   = 0;
}

WorldBorderS2CPacket::WorldBorderS2CPacket(const WorldBorder &border)
{
    m_centerX       = border.GetCenterX();
    m_centerZ       = border.GetCenterZ();
    m_size          = border.GetSize();
    m_lerpTarget    = border.GetLerpTarget();
    m_lerpTicks     = border.GetLerpRemainingTicks();
    m_warningBlocks = border.GetWarningBlocks();
    m_warningTime   = border.GetWarningTime();
}

WorldBorderS2CPacket::~WorldBorderS2CPacket()
{}

ePACKET_ID WorldBorderS2CPacket::GetId()
{
    return ePacketId_WorldBorderS2C;
}

void WorldBorderS2CPacket::Read(DataInputStream *inputStream)
{
    const double maxCoordinate = 30000000.0;
    const double maxSize       = 60000000.0;

    m_centerX       = inputStream->ReadDouble();
    m_centerZ       = inputStream->ReadDouble();
    m_size          = inputStream->ReadDouble();
    m_lerpTarget    = inputStream->ReadDouble();
    m_lerpTicks     = (int64_t) inputStream->ReadLong();
    m_warningBlocks = (int) inputStream->ReadInt();
    m_warningTime   = (int) inputStream->ReadInt();
    m_centerX       = std::isfinite(m_centerX) ? std::clamp(m_centerX, -maxCoordinate, maxCoordinate) : 0.0;
    m_centerZ       = std::isfinite(m_centerZ) ? std::clamp(m_centerZ, -maxCoordinate, maxCoordinate) : 0.0;
    m_size          = std::isfinite(m_size) ? std::clamp(m_size, 1.0, maxSize) : maxSize;
    m_lerpTarget    = std::isfinite(m_lerpTarget) ? std::clamp(m_lerpTarget, 1.0, maxSize) : m_size;
    m_lerpTicks     = std::max(m_lerpTicks, (int64_t) 0);
    m_warningBlocks = std::max(m_warningBlocks, 0);
    m_warningTime   = std::max(m_warningTime, 0);
}

void WorldBorderS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteDouble(m_centerX);
    outputStream->WriteDouble(m_centerZ);
    outputStream->WriteDouble(m_size);
    outputStream->WriteDouble(m_lerpTarget);
    outputStream->WriteLong((uint64_t) m_lerpTicks);
    outputStream->WriteInt((uint32_t) m_warningBlocks);
    outputStream->WriteInt((uint32_t) m_warningTime);
}

void WorldBorderS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleWorldBorder(this);
}

int WorldBorderS2CPacket::GetEstimatedSize()
{
    return 48;
}

void WorldBorderS2CPacket::Apply(WorldBorder *border) const
{
    border->SetCenter(m_centerX, m_centerZ);
    border->LerpSizeBetween(m_size, m_lerpTarget, m_lerpTicks);
    border->SetWarningBlocks(m_warningBlocks);
    border->SetWarningTime(m_warningTime);
}
