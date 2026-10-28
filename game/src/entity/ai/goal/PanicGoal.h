#pragma once

#include "Goal.h"

class Mob;

class PanicGoal : public Goal
{
public:
    PanicGoal(Mob *mob, double speed);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Start() override;

private:
    Mob *m_mob;
    double m_speed;
    double m_posX;
    double m_posY;
    double m_posZ;
};
