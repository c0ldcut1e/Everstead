#pragma once

#include "Goal.h"

class Skeleton;

class RangedAttackGoal : public Goal
{
public:
    RangedAttackGoal(Skeleton *mob, double speed, int attackIntervalMin, int attackIntervalMax, float attackRadius);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Stop() override;
    void Tick() override;

private:
    Skeleton *m_mob;
    double m_speed;
    int m_attackIntervalMin;
    int m_attackIntervalMax;
    float m_attackRadius;
    int m_attackTime;
    int m_seeTime;
};
