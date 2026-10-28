#pragma once

#include "../Entity.h"

class ExperienceOrb : public Entity
{
public:
    explicit ExperienceOrb(Level *level);
    ~ExperienceOrb() override;

    eENTITY_TYPE GetType() override;
    bool IsPickable() const override;
    void Tick() override;
    int GetUpdateInterval() const override;
    bool SendsVelocity() const override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;

    int GetValue() const;
    int GetIcon() const;
    bool TakeTargetChanged();

    static int GetExperienceValue(int maxValue);
    static void SpawnOrbs(Level *level, const glm::dvec3 &position, int amount);

    static const eENTITY_TYPE s_type   = eEntityType_ExperienceOrb;
    static const int s_lifetime        = 6000;
    static constexpr int kValueTiers[] = {2477, 1237, 617, 307, 149, 73, 37, 17, 7, 3};

private:
    int m_value;
    int m_age;
    const Player *m_followTarget;
    bool m_targetChanged;
};
