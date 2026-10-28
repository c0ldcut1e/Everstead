#pragma once

#include "Animal.h"

class Cow : public Animal
{
public:
    explicit Cow(Level *level);

    eENTITY_TYPE GetType() override;
    bool Interact(Player *player) override;
    std::unique_ptr<AgeableMob> CreateOffspring(AgeableMob *partner) override;

    static const eENTITY_TYPE s_type = eEntityType_Cow;

protected:
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;
    float GetSoundVolume() const override;
};
