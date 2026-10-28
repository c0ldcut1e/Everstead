#pragma once

#include "Goal.h"

class Item;
class Mob;

class TemptGoal : public Goal
{
public:
    TemptGoal(Mob *mob, double speed, Item *item);

    bool CanUse() override;
    void Start() override;
    void Stop() override;
    void Tick() override;

private:
    Mob *m_mob;
    double m_speed;
    Item *m_item;
    int m_playerId;
    int m_calmDown;
    bool m_oldAvoidWater;
};
