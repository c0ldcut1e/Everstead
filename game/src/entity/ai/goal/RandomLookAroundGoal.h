#pragma once

#include "Goal.h"

class Mob;

class RandomLookAroundGoal : public Goal
{
public:
    explicit RandomLookAroundGoal(Mob *mob);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Start() override;
    void Tick() override;

private:
    Mob *m_mob;
    double m_relX;
    double m_relZ;
    int m_lookTime;
};
