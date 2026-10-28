#pragma once

#include "Goal.h"

class Mob;

class RandomStrollGoal : public Goal
{
public:
    RandomStrollGoal(Mob *mob, double speed);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Start() override;

private:
    Mob *m_mob;
    double m_speed;
    double m_wantedX;
    double m_wantedY;
    double m_wantedZ;
};
