#pragma once

#include "../Mob.h"

class Monster : public Mob
{
public:
    explicit Monster(Level *level);

    float GetWalkTargetValue(const BlockPos &pos) const override;
    bool CanBeLeashed() const override;

protected:
    void ServerTick() override;
    int GetExperienceReward() override;
    bool RemoveWhenFarAway() const override;
};
