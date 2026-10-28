#pragma once

#include "Monster.h"

class Skeleton : public Monster
{
public:
    explicit Skeleton(Level *level);

    eENTITY_TYPE GetType() override;
    void PerformRangedAttack(LivingEntity *target);

    static const eENTITY_TYPE s_type = eEntityType_Skeleton;

protected:
    void ServerTick() override;
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;
};
