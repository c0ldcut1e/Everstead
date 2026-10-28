#pragma once

#include "Monster.h"

class Slime : public Monster
{
public:
    explicit Slime(Level *level);

    eENTITY_TYPE GetType() override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void GetMobData(MobData *data) const override;
    void ApplyMobData(const MobData &data) override;
    int GetSlimeSize() const;
    void SetSlimeSize(int size);
    float GetSquish(float partialTick) const;

    static const eENTITY_TYPE s_type = eEntityType_Slime;

protected:
    void ServerTick() override;
    void ServerAiStep() override;
    void ClientTick() override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;
    float GetSoundVolume() const override;
    void GetLootProperties(std::vector<LootProperty> *properties) const override;
    int GetExperienceReward() override;

private:
    void Split();

    int m_size;
    int m_jumpDelay;
    bool m_wasOnGround;
    float m_squish;
    float m_oldSquish;
    float m_targetSquish;
};
