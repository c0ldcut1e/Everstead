#pragma once

#include <cstdint>

#include "TargetGoal.h"

class HurtByTargetGoal : public TargetGoal
{
public:
    explicit HurtByTargetGoal(Mob *mob);

    bool CanUse() override;
    void Start() override;

private:
    int64_t m_timestamp;
};
