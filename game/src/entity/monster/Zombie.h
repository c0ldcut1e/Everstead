#pragma once

#include "Monster.h"

class Zombie : public Monster
{
public:
    explicit Zombie(Level *level);

    eENTITY_TYPE GetType() override;
    void FinalizeSpawn() override;
    bool IsBaby() const override;
    void SetBaby(bool baby);
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void GetMobData(MobData *data) const override;
    void ApplyMobData(const MobData &data) override;

    static const eENTITY_TYPE s_type = eEntityType_Zombie;

protected:
    void ServerTick() override;
    bool CanDropLoot() const override;
    int GetExperienceReward() override;
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;

private:
    bool m_baby;
};
