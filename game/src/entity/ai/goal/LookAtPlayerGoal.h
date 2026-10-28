#pragma once

#include "Goal.h"

class Mob;

class LookAtPlayerGoal : public Goal
{
public:
    LookAtPlayerGoal(Mob *mob, float lookDistance);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Start() override;
    void Stop() override;
    void Tick() override;

private:
    Mob *m_mob;
    float m_lookDistance;
    int m_lookAtId;
    int m_lookTime;
};
