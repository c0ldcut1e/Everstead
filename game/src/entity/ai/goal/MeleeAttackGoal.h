#pragma once

#include "Goal.h"

class Mob;

class MeleeAttackGoal : public Goal
{
public:
    MeleeAttackGoal(Mob *mob, double speed, bool trackTarget);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Start() override;
    void Stop() override;
    void Tick() override;

private:
    Mob *m_mob;
    double m_speed;
    bool m_trackTarget;
    int m_timeToRecalcPath;
};
