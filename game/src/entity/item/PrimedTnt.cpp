#include "PrimedTnt.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../world/Explosion.h"
#include "../../world/Level.h"

PrimedTnt::PrimedTnt(Level *level) : Entity(level)
{
    m_blocksBuilding = true;
    m_liquidInsetY   = 0.001;
    m_fuse           = 80;
    m_explosionPower = 4.0f;
    m_noGravity      = true;
    m_boundingBox    = {glm::dvec3(-0.49, 0.0, -0.49), glm::dvec3(0.49, 0.98, 0.49)};
}

PrimedTnt::~PrimedTnt()
{}

eENTITY_TYPE PrimedTnt::GetType()
{
    return s_type;
}

void PrimedTnt::Tick()
{
    m_totalTickCount++;
    UpdateAABB();
    if (IsInWater())
    {
        ApplyWaterFlow();
    }

    m_velocity.y -= 0.04;
    Move(m_velocity.x, m_velocity.y, m_velocity.z);
    m_velocity *= 0.98;
    if (m_onGround)
    {
        m_velocity.x *= 0.7;
        m_velocity.z *= 0.7;
        m_velocity.y *= -0.5;
    }

    if (m_fuse-- <= 0)
    {
        if (!m_level->IsClientSide())
        {
            Explosion explosion(m_level, glm::dvec3(m_position.x, m_position.y + 0.49, m_position.z), m_explosionPower);
            explosion.Explode();
        }

        m_level->RemoveEntity(this);
        return;
    }

    if (m_level->IsClientSide())
    {
        m_level->SpawnParticle(eParticleType_Smoke, glm::dvec3(m_position.x, m_position.y + 0.99, m_position.z));
    }
}

int PrimedTnt::GetUpdateInterval() const
{
    return 10;
}

bool PrimedTnt::SendsVelocity() const
{
    return true;
}

void PrimedTnt::ReadSpawnData(int data)
{
    m_fuse = data;
}

int PrimedTnt::GetSpawnData()
{
    return m_fuse;
}

void PrimedTnt::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    if (tag->Contains("Fuse"))
    {
        m_fuse = tag->GetShort("Fuse");
    }

    if (tag->Contains("explosion_power"))
    {
        m_explosionPower = std::clamp(tag->GetFloat("explosion_power"), 0.0f, 128.0f);
    }
}

void PrimedTnt::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    tag->PutShort("Fuse", (int16_t) m_fuse);
    tag->PutFloat("explosion_power", m_explosionPower);
}

void PrimedTnt::Ignite(const glm::dvec3 &position, int fuse)
{
    double rot = m_level->GetRandom().NextFloat() * std::numbers::pi * 2.0;
    SetPosition(position);
    SetVelocity(glm::dvec3(-sin(rot) * 0.02, 0.2, -cos(rot) * 0.02));
    m_fuse = fuse;
}

int PrimedTnt::GetFuse() const
{
    return m_fuse;
}
