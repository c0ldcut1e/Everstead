#pragma once

#include <cstdint>
#include <vector>

#include "../utils/AABB.h"
#include "block/BlockPos.h"

enum eWORLD_BORDER_STATUS : uint8_t
{
    eWorldBorderStatus_Static    = 0,
    eWorldBorderStatus_Growing   = 1,
    eWorldBorderStatus_Shrinking = 2,
};

class WorldBorder
{
public:
    WorldBorder();
    ~WorldBorder();

    void Tick();

    double GetCenterX() const;
    double GetCenterZ() const;
    void SetCenter(double x, double z);
    double GetSize() const;
    double GetSize(float partialTicks) const;
    double GetLerpTarget() const;
    int64_t GetLerpRemainingTicks() const;
    double GetLerpSpeed() const;
    eWORLD_BORDER_STATUS GetStatus() const;
    void SetSize(double size);
    void LerpSizeBetween(double from, double to, int64_t ticks);

    double GetMinX() const;
    double GetMaxX() const;
    double GetMinZ() const;
    double GetMaxZ() const;
    bool IsWithinBounds(const BlockPos &pos) const;
    double GetDistanceToBorder(double x, double z) const;
    void GetCollisionBoxes(const glm::dvec3 &position, const AABB &sweep, std::vector<AABB> *boxes) const;

    double GetDamagePerBlock() const;
    void SetDamagePerBlock(double damagePerBlock);
    double GetDamageBuffer() const;
    void SetDamageBuffer(double damageBuffer);
    int GetWarningBlocks() const;
    void SetWarningBlocks(int warningBlocks);
    int GetWarningTime() const;
    void SetWarningTime(int warningTime);

private:
    double m_centerX;
    double m_centerZ;
    double m_size;
    double m_previousSize;
    double m_lerpFrom;
    double m_lerpTo;
    int64_t m_lerpDuration;
    int64_t m_lerpRemaining;
    double m_damagePerBlock;
    double m_damageBuffer;
    int m_warningBlocks;
    int m_warningTime;
};
