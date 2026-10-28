#include "MobSpawnerBlockEntity.h"

#include <cmath>
#include <utility>

#include "../../entity/EntityFactory.h"
#include "../../entity/monster/Monster.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../tag/CompoundTag.h"
#include "../ServerLevel.h"

#include <glm/gtx/norm.hpp>

MobSpawnerBlockEntity::MobSpawnerBlockEntity(Level *level, const BlockPos &pos) : BlockEntity(level, pos)
{
    m_entityType    = eEntityType_Pig;
    m_spawnDelay    = 20;
    m_minSpawnDelay = 200;
    m_maxSpawnDelay = 800;
    m_spawnCount    = 4;
    m_spin          = 0.0f;
    m_oldSpin       = 0.0f;
}

MobSpawnerBlockEntity::~MobSpawnerBlockEntity() = default;

const char *MobSpawnerBlockEntity::GetId() const
{
    return "mob_spawner";
}

bool MobSpawnerBlockEntity::IsNearPlayer() const
{
    glm::dvec3 center(m_pos.x + 0.5, m_pos.y + 0.5, m_pos.z + 0.5);
    if (m_level->IsClientSide())
    {
        return glm::distance2(m_level->GetLocalPlayer()->GetPosition(), center) < 16.0 * 16.0;
    }

    return m_level->FindNearestPlayer(center, 16.0) != nullptr;
}

void MobSpawnerBlockEntity::Delay()
{
    m_spawnDelay = m_minSpawnDelay + m_level->GetRandom().NextInt(0, m_maxSpawnDelay - m_minSpawnDelay - 1);
}

void MobSpawnerBlockEntity::Tick()
{
    if (!IsNearPlayer())
    {
        return;
    }

    if (!m_level->IsClientSide())
    {
        TickServer();
        return;
    }

    glm::dvec3 position(m_pos.x + m_level->GetRandom().NextFloat(), m_pos.y + m_level->GetRandom().NextFloat(),
                        m_pos.z + m_level->GetRandom().NextFloat());
    m_level->SpawnParticle(eParticleType_Smoke, position);
    m_level->SpawnParticle(eParticleType_Flame, position);

    m_oldSpin = m_spin;
    m_spin    = fmodf(m_spin + 1000.0f / (m_spawnDelay + 200.0f), 360.0f);
}

void MobSpawnerBlockEntity::TickServer()
{
    if (m_spawnDelay == -1)
    {
        Delay();
    }

    if (m_spawnDelay > 0)
    {
        m_spawnDelay--;
        return;
    }

    ServerLevel *level                                   = (ServerLevel *) m_level;
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    AABB nearbyBox(glm::dvec3(m_pos.x - 8.0, m_pos.y - 4.0, m_pos.z - 8.0), glm::dvec3(m_pos.x + 9.0, m_pos.y + 5.0, m_pos.z + 9.0));
    for (int i = 0; i < m_spawnCount; i++)
    {
        int nearby = 0;
        for (size_t j = 0; j < entities.size(); j++)
        {
            if (entities[j]->GetType() == m_entityType && entities[j]->GetAABB().Intersects(nearbyBox))
            {
                nearby++;
            }
        }

        if (nearby >= 6)
        {
            Delay();
            return;
        }

        int monsters = 0;
        for (size_t j = 0; j < entities.size(); j++)
        {
            if (dynamic_cast<Monster *>(entities[j].get()))
            {
                monsters++;
            }
        }

        if (monsters >= 60)
        {
            return;
        }

        std::unique_ptr<Entity> entity = EntityFactory::Create(m_entityType, level);
        glm::dvec3 position(m_pos.x + (m_level->GetRandom().NextDouble() - m_level->GetRandom().NextDouble()) * 4.0,
                            m_pos.y + m_level->GetRandom().NextInt(0, 2) - 1,
                            m_pos.z + (m_level->GetRandom().NextDouble() - m_level->GetRandom().NextDouble()) * 4.0);
        entity->SetPosition(position);
        entity->SetRotation(m_level->GetRandom().NextFloat() * 360.0f, 0.0f);
        entity->ReadSpawnData(EntityFactory::CreateSpawnData(m_entityType, &m_level->GetRandom()));
        Mob *mob = dynamic_cast<Mob *>(entity.get());
        if (!mob)
        {
            level->AddEntity(std::move(entity));
            Delay();
        }
        else if (level->CanMobSpawnAt(mob, BlockPos((int) floor(position.x), (int) floor(position.y), (int) floor(position.z))))
        {
            level->AddEntity(std::move(entity));
            mob->FinalizeSpawn();
            Delay();
        }
    }
}

void MobSpawnerBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);
    tag->PutString("EntityId", EntityFactory::GetResourceLocation(m_entityType).ToString());
    tag->PutShort("Delay", (int16_t) m_spawnDelay);
    tag->PutShort("MinSpawnDelay", (int16_t) m_minSpawnDelay);
    tag->PutShort("MaxSpawnDelay", (int16_t) m_maxSpawnDelay);
    tag->PutShort("SpawnCount", (int16_t) m_spawnCount);
}

void MobSpawnerBlockEntity::Load(const CompoundTag *tag)
{
    if (EntityFactory::ByName(tag->GetString("EntityId"), &m_entityType))
    {
        m_displayEntity.reset();
    }

    if (tag->Contains("Delay"))
    {
        m_spawnDelay = tag->GetShort("Delay");
    }

    if (tag->Contains("MinSpawnDelay"))
    {
        m_minSpawnDelay = tag->GetShort("MinSpawnDelay");
    }

    if (tag->Contains("MaxSpawnDelay"))
    {
        m_maxSpawnDelay = tag->GetShort("MaxSpawnDelay");
    }

    if (tag->Contains("SpawnCount"))
    {
        m_spawnCount = tag->GetShort("SpawnCount");
    }
}

eENTITY_TYPE MobSpawnerBlockEntity::GetEntityType() const
{
    return m_entityType;
}

void MobSpawnerBlockEntity::SetEntityType(eENTITY_TYPE entityType)
{
    m_entityType = entityType;
    m_displayEntity.reset();
}

Entity *MobSpawnerBlockEntity::GetDisplayEntity()
{
    if (!m_displayEntity)
    {
        m_displayEntity = EntityFactory::Create(m_entityType, m_level);
    }

    return m_displayEntity.get();
}

float MobSpawnerBlockEntity::GetSpin(float partialTicks) const
{
    return m_oldSpin + (m_spin - m_oldSpin) * partialTicks;
}
