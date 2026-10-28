#pragma once

#include "Goal.h"

class Animal;

class BreedGoal : public Goal
{
public:
    BreedGoal(Animal *animal, double speed);

    bool CanUse() override;
    bool CanContinueToUse() override;
    void Stop() override;
    void Tick() override;

private:
    Animal *GetPartner() const;
    Animal *FindFreePartner() const;
    void Breed(Animal *partner);

    Animal *m_animal;
    double m_speed;
    int m_partnerId;
    int m_loveTime;
};
