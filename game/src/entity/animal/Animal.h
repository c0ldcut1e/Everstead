#pragma once

#include "../AgeableMob.h"

class Animal : public AgeableMob
{
public:
    explicit Animal(Level *level);

    bool Interact(Player *player) override;
    bool Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker) override;
    float GetWalkTargetValue(const BlockPos &pos) const override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void GetMobData(MobData *data) const override;
    void ApplyMobData(const MobData &data) override;

    virtual bool IsFood(const ItemInstance &item) const;
    bool IsInLove() const;
    void ResetLove();
    bool CanMate(Animal *other);

protected:
    void ServerTick() override;
    void ClientTick() override;
    int GetAmbientSoundInterval() const override;
    int GetExperienceReward() override;

    int m_inLove;
};
