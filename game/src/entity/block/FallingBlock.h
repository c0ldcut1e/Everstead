#pragma once

#include <cstdint>

#include "../Entity.h"

class Block;

class FallingBlock : public Entity
{
public:
    explicit FallingBlock(Level *level);
    ~FallingBlock() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;
    int GetUpdateInterval() const override;
    bool SendsVelocity() const override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;

    Block *GetBlock() const;
    uint16_t GetBlockData() const;

    static const eENTITY_TYPE s_type = eEntityType_FallingBlock;

private:
    void Land();

    Block *m_block;
    uint16_t m_blockData;
    Random m_random;
    int m_spawnData;
    float m_fallDistance;
};
