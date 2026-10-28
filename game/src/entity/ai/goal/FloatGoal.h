#pragma once

#include "Goal.h"

class Mob;

class FloatGoal : public Goal
{
public:
    explicit FloatGoal(Mob *mob);

    bool CanUse() override;
    void Tick() override;

private:
    Mob *m_mob;
};
