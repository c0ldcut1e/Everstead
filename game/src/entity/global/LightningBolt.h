#pragma once

#include <cstdint>

#include "../Entity.h"

class LightningBolt : public Entity
{
public:
    explicit LightningBolt(Level *level);
    ~LightningBolt() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;

    int64_t GetSeed() const;
    bool IsVisible() const;

    static const eENTITY_TYPE s_type = eEntityType_LightningBolt;

private:
    void SpawnFire();
    void StrikeEntities();

    int m_spawnData;
    Random m_random;
    int64_t m_seed;
    int m_life;
    int m_flashes;
};
