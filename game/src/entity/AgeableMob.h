#pragma once

#include <memory>

#include "Mob.h"

class AgeableMob : public Mob
{
public:
    explicit AgeableMob(Level *level);

    bool Interact(Player *player) override;
    bool IsBaby() const override;
    void SetSize(float width, float height) override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void GetMobData(MobData *data) const override;
    void ApplyMobData(const MobData &data) override;

    virtual std::unique_ptr<AgeableMob> CreateOffspring(AgeableMob *partner) = 0;
    int GetAge() const;
    void SetAge(int age);

protected:
    void ServerTick() override;

    int m_age;
    float m_adultWidth;
    float m_adultHeight;
};
