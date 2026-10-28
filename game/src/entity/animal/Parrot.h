#pragma once

#include "Animal.h"

class Parrot : public Animal
{
public:
    explicit Parrot(Level *level);

    eENTITY_TYPE GetType() override;
    std::unique_ptr<AgeableMob> CreateOffspring(AgeableMob *partner) override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void GetMobData(MobData *data) const override;
    void ApplyMobData(const MobData &data) override;

    int GetVariant() const;
    float GetFlap(float partialTicks) const;
    float GetFlapSpeed(float partialTicks) const;

    static const eENTITY_TYPE s_type          = eEntityType_Parrot;
    static constexpr const char *kVariants[5] = {"red_blue", "blue", "green", "yellow_blue", "grey"};
    static const int kVariantCount            = 5;

protected:
    void ServerTick() override;
    void ClientTick() override;
    void CauseFallDamage(float distance) override;
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;

private:
    float m_flap;
    float m_oldFlap;
    float m_flapSpeed;
    float m_oldFlapSpeed;
    float m_flapping;
    int m_variant;
};
