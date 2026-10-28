#pragma once

#include "Goal.h"

class AgeableMob;

class FollowParentGoal : public Goal
{
public:
    FollowParentGoal(AgeableMob *mob, double speed);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Start() override;
    void Stop() override;
    void Tick() override;

private:
    AgeableMob *m_mob;
    double m_speed;
    int m_parentId;
    int m_timeToRecalcPath;
};
