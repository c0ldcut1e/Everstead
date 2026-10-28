#pragma once

#include <memory>

#include "../../entity/EntityType.h"
#include "../../utils/Random.h"
#include "BlockEntity.h"

class Entity;

class MobSpawnerBlockEntity : public BlockEntity
{
public:
    MobSpawnerBlockEntity(Level *level, const BlockPos &pos);
    ~MobSpawnerBlockEntity() override;

    const char *GetId() const override;
    void Tick() override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;

    eENTITY_TYPE GetEntityType() const;
    void SetEntityType(eENTITY_TYPE entityType);
    Entity *GetDisplayEntity();
    float GetSpin(float partialTicks) const;

private:
    bool IsNearPlayer() const;
    void Delay();
    void TickServer();

    eENTITY_TYPE m_entityType;
    int m_spawnDelay;
    int m_minSpawnDelay;
    int m_maxSpawnDelay;
    int m_spawnCount;
    float m_spin;
    float m_oldSpin;
    std::unique_ptr<Entity> m_displayEntity;
};
