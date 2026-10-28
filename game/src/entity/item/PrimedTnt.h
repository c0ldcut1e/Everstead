#pragma once

#include "../Entity.h"

class PrimedTnt : public Entity
{
public:
    explicit PrimedTnt(Level *level);
    ~PrimedTnt() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;
    int GetUpdateInterval() const override;
    bool SendsVelocity() const override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;

    void Ignite(const glm::dvec3 &position, int fuse);
    int GetFuse() const;

    static const eENTITY_TYPE s_type = eEntityType_PrimedTnt;

private:
    int m_fuse;
    float m_explosionPower;
};
