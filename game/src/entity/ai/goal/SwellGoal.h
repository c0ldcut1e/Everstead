#pragma once

#include "Goal.h"

class Creeper;

class SwellGoal : public Goal
{
public:
    explicit SwellGoal(Creeper *creeper);

    bool CanUse() override;
    void Start() override;
    void Tick() override;

private:
    Creeper *m_creeper;
};
