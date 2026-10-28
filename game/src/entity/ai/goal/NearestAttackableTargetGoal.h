#pragma once

#include "TargetGoal.h"

class NearestAttackableTargetGoal : public TargetGoal
{
public:
    NearestAttackableTargetGoal(Mob *mob, double within, bool mustSee);

    bool CanUse() override;
    void Start() override;

private:
    int m_targetId;
};
