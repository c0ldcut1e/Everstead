#include "LightningBolt.h"

#include <cmath>
#include <string>

#include "../../world/Level.h"
#include "../../world/block/Block.h"
#include "../LivingEntity.h"

LightningBolt::LightningBolt(Level *level) : Entity(level)
{
    m_spawnData   = 0;
    m_seed        = 0;
    m_life        = 2;
    m_flashes     = 0;
    m_noGravity   = true;
    m_noCollision = true;
    m_boundingBox = {glm::dvec3(-16.0, 0.0, -16.0), glm::dvec3(16.0, 128.0, 16.0)};
}

LightningBolt::~LightningBolt()
{}

eENTITY_TYPE LightningBolt::GetType()
{
    return s_type;
}

void LightningBolt::Tick()
{
    bool newFlash = m_totalTickCount < 0;
    if (newFlash && m_level->IsClientSide())
    {
        m_level->PlayThunder(m_position, 1.0f, 0.8f + m_random.NextFloat() * 0.2f);
        m_level->PlaySound("random.explode" + std::to_string(m_random.NextInt(1, 4)), m_position, 2.0f, 0.5f + m_random.NextFloat() * 0.2f);
    }

    if (newFlash && !m_level->IsClientSide())
    {
        SpawnFire();
    }

    m_totalTickCount++;
    m_life--;
    if (m_life < 0)
    {
        if (m_flashes == 0)
        {
            m_level->RemoveEntity(this);
            return;
        }

        if (m_life < -m_random.NextInt(0, 9))
        {
            m_flashes--;
            m_life   = 1;
            newFlash = true;
            m_seed   = (int64_t) m_random.NextUInt64();
        }
    }

    if (newFlash)
    {
        m_level->AddLightningFlash(m_position, 1.0f);
    }

    if (m_life >= 0 && !m_level->IsClientSide())
    {
        StrikeEntities();
    }
}

void LightningBolt::SpawnFire()
{
    BlockPos origin((int) floor(m_position.x), (int) floor(m_position.y), (int) floor(m_position.z));
    if (m_level->GetDifficulty() < 2 || !m_level->IsAreaLoaded(origin, 10))
    {
        return;
    }

    Block *fire = Block::ByName("fire");
    for (int i = 0; i < 5; i++)
    {
        BlockPos pos = origin;
        if (i > 0)
        {
            pos = BlockPos(origin.x + m_random.NextInt(0, 2) - 1, origin.y + m_random.NextInt(0, 2) - 1, origin.z + m_random.NextInt(0, 2) - 1);
        }

        if (m_level->GetBlockId(pos) == 0 && fire->CanSurvive(m_level, pos, nullptr))
        {
            m_level->SetBlock(pos, fire);
            m_level->RecordBlockChange(pos);
        }
    }
}

void LightningBolt::StrikeEntities()
{
    AABB area(m_position + glm::dvec3(-3.0, -3.0, -3.0), m_position + glm::dvec3(3.0, 9.0, 3.0));
    for (const std::unique_ptr<Entity> &entity : m_level->GetEntities())
    {
        LivingEntity *living = dynamic_cast<LivingEntity *>(entity.get());
        if (living && living->GetAABB().Intersects(area))
        {
            living->Hurt(eDamageSource_LightningBolt, 5, nullptr);
            living->SetOnFire(8);
        }
    }
}

void LightningBolt::ReadSpawnData(int data)
{
    m_spawnData = data;
    m_random    = Random((uint64_t) (uint32_t) data);
    m_seed      = (int64_t) m_random.NextUInt64();
    m_flashes   = m_random.NextInt(1, 3);
}

int LightningBolt::GetSpawnData()
{
    return m_spawnData;
}

int64_t LightningBolt::GetSeed() const
{
    return m_seed;
}

bool LightningBolt::IsVisible() const
{
    return m_life >= 0;
}
