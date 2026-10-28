#pragma once

#include "Animal.h"

class Chicken : public Animal
{
public:
    explicit Chicken(Level *level);

    eENTITY_TYPE GetType() override;
    std::unique_ptr<AgeableMob> CreateOffspring(AgeableMob *partner) override;
    bool IsFood(const ItemInstance &item) const override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;

    void SetChickenJockey(bool chickenJockey);
    float GetFlap(float partialTicks) const;
    float GetFlapSpeed(float partialTicks) const;

    static const eENTITY_TYPE s_type = eEntityType_Chicken;

protected:
    void ServerTick() override;
    void ClientTick() override;
    void CauseFallDamage(float distance) override;
    bool RemoveWhenFarAway() const override;
    int GetExperienceReward() override;
    double GetRideHeight() const override;
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;

private:
    float m_flap;
    float m_oldFlap;
    float m_flapSpeed;
    float m_oldFlapSpeed;
    float m_flapping;
    int m_eggTime;
    bool m_chickenJockey;
};
