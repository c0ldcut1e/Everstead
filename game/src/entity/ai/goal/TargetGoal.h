#pragma once

#include "Goal.h"

class Mob;

class TargetGoal : public Goal
{
public:
    TargetGoal(Mob *mob, double within, bool mustSee);

    bool CanContinueToUse() override;
    void Start() override;
    void Stop() override;

protected:
    Mob *m_mob;
    double m_within;
    bool m_mustSee;
    int m_unseenTicks;
};
