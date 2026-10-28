#include "WorldBorder.h"

#include <algorithm>
#include <cmath>

WorldBorder::WorldBorder()
{
    m_centerX        = 0.0;
    m_centerZ        = 0.0;
    m_damagePerBlock = 0.2;
    m_damageBuffer   = 5.0;
    m_warningBlocks  = 5;
    m_warningTime    = 15;
    SetSize(31101400.0);
}

WorldBorder::~WorldBorder()
{}

void WorldBorder::Tick()
{
    m_previousSize = m_size;
    if (m_lerpRemaining > 0)
    {
        m_lerpRemaining--;
        m_size = m_lerpTo + (m_lerpFrom - m_lerpTo) * (double) m_lerpRemaining / (double) m_lerpDuration;
    }
}

double WorldBorder::GetCenterX() const
{
    return m_centerX;
}

double WorldBorder::GetCenterZ() const
{
    return m_centerZ;
}

void WorldBorder::SetCenter(double x, double z)
{
    m_centerX = x;
    m_centerZ = z;
}

double WorldBorder::GetSize() const
{
    return m_size;
}

double WorldBorder::GetSize(float partialTicks) const
{
    return m_previousSize + (m_size - m_previousSize) * partialTicks;
}

double WorldBorder::GetLerpTarget() const
{
    return m_lerpTo;
}

int64_t WorldBorder::GetLerpRemainingTicks() const
{
    return m_lerpRemaining;
}

double WorldBorder::GetLerpSpeed() const
{
    if (m_lerpRemaining <= 0)
    {
        return 0.0;
    }

    return fabs(m_lerpFrom - m_lerpTo) / (double) m_lerpDuration;
}

eWORLD_BORDER_STATUS WorldBorder::GetStatus() const
{
    if (m_lerpRemaining <= 0)
    {
        return eWorldBorderStatus_Static;
    }

    return m_lerpTo < m_lerpFrom ? eWorldBorderStatus_Shrinking : eWorldBorderStatus_Growing;
}

void WorldBorder::SetSize(double size)
{
    m_size          = size;
    m_previousSize  = size;
    m_lerpFrom      = size;
    m_lerpTo        = size;
    m_lerpDuration  = 0;
    m_lerpRemaining = 0;
}

void WorldBorder::LerpSizeBetween(double from, double to, int64_t ticks)
{
    if (ticks <= 0 || from == to)
    {
        SetSize(to);
        return;
    }

    m_size          = from;
    m_previousSize  = from;
    m_lerpFrom      = from;
    m_lerpTo        = to;
    m_lerpDuration  = ticks;
    m_lerpRemaining = ticks;
}

double WorldBorder::GetMinX() const
{
    return m_centerX - m_size * 0.5;
}

double WorldBorder::GetMaxX() const
{
    return m_centerX + m_size * 0.5;
}

double WorldBorder::GetMinZ() const
{
    return m_centerZ - m_size * 0.5;
}

double WorldBorder::GetMaxZ() const
{
    return m_centerZ + m_size * 0.5;
}

bool WorldBorder::IsWithinBounds(const BlockPos &pos) const
{
    return (double) pos.x + 1.0 > GetMinX() && (double) pos.x < GetMaxX() && (double) pos.z + 1.0 > GetMinZ() && (double) pos.z < GetMaxZ();
}

double WorldBorder::GetDistanceToBorder(double x, double z) const
{
    return std::min(std::min(x - GetMinX(), GetMaxX() - x), std::min(z - GetMinZ(), GetMaxZ() - z));
}

void WorldBorder::GetCollisionBoxes(const glm::dvec3 &position, const AABB &sweep, std::vector<AABB> *boxes) const
{
    const glm::dvec3 &min = sweep.GetMin();
    const glm::dvec3 &max = sweep.GetMax();
    double margin   = std::max(std::max(max.x - min.x, max.z - min.z), 1.0);
    double minX     = GetMinX();
    double maxX     = GetMaxX();
    double minZ     = GetMinZ();
    double maxZ     = GetMaxZ();
    if (GetDistanceToBorder(position.x, position.z) >= margin * 2.0 || position.x < minX - margin || position.x >= maxX + margin ||
        position.z < minZ - margin || position.z >= maxZ + margin)
    {
        return;
    }

    // The walls only need to cover the swept area, so each one is clipped to it with a block of slack
    double bottom = min.y - 1.0;
    double top    = max.y + 1.0;
    double left   = std::min(min.x, minX) - 1.0;
    double right  = std::max(max.x, maxX) + 1.0;
    double back   = std::min(min.z, minZ) - 1.0;
    double front  = std::max(max.z, maxZ) + 1.0;
    boxes->push_back(AABB(glm::dvec3(left, bottom, min.z - 1.0), glm::dvec3(minX, top, max.z + 1.0)));
    boxes->push_back(AABB(glm::dvec3(maxX, bottom, min.z - 1.0), glm::dvec3(right, top, max.z + 1.0)));
    boxes->push_back(AABB(glm::dvec3(min.x - 1.0, bottom, back), glm::dvec3(max.x + 1.0, top, minZ)));
    boxes->push_back(AABB(glm::dvec3(min.x - 1.0, bottom, maxZ), glm::dvec3(max.x + 1.0, top, front)));
}

double WorldBorder::GetDamagePerBlock() const
{
    return m_damagePerBlock;
}

void WorldBorder::SetDamagePerBlock(double damagePerBlock)
{
    m_damagePerBlock = damagePerBlock;
}

double WorldBorder::GetDamageBuffer() const
{
    return m_damageBuffer;
}

void WorldBorder::SetDamageBuffer(double damageBuffer)
{
    m_damageBuffer = damageBuffer;
}

int WorldBorder::GetWarningBlocks() const
{
    return m_warningBlocks;
}

void WorldBorder::SetWarningBlocks(int warningBlocks)
{
    m_warningBlocks = warningBlocks;
}

int WorldBorder::GetWarningTime() const
{
    return m_warningTime;
}

void WorldBorder::SetWarningTime(int warningTime)
{
    m_warningTime = warningTime;
}
