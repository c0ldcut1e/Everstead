#pragma once

#include "Goal.h"

class Sheep;

class EatTileGoal : public Goal
{
public:
    explicit EatTileGoal(Sheep *sheep);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Start() override;
    void Stop() override;
    void Tick() override;

    int GetEatAnimationTick() const;

private:
    Sheep *m_sheep;
    int m_eatAnimationTick;
};
