#pragma once

#include "Animal.h"

class Ocelot : public Animal
{
public:
    explicit Ocelot(Level *level);

    eENTITY_TYPE GetType() override;
    std::unique_ptr<AgeableMob> CreateOffspring(AgeableMob *partner) override;

    static const eENTITY_TYPE s_type = eEntityType_Ocelot;

protected:
    void CauseFallDamage(float distance) override;
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;
};
