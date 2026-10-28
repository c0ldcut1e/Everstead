#pragma once

#include "Animal.h"

class Pig : public Animal
{
public:
    explicit Pig(Level *level);

    eENTITY_TYPE GetType() override;
    std::unique_ptr<AgeableMob> CreateOffspring(AgeableMob *partner) override;
    bool IsFood(const ItemInstance &item) const override;

    static const eENTITY_TYPE s_type = eEntityType_Pig;

protected:
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;
};
