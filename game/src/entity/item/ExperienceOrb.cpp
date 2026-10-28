#include "ExperienceOrb.h"

#include <cmath>
#include <iterator>

#include "../../world/Level.h"
#include "../player/Player.h"

ExperienceOrb::ExperienceOrb(Level *level) : Entity(level)
{
    m_liquidInsetY  = 0.001;
    m_value         = 0;
    m_age           = 0;
    m_followTarget  = nullptr;
    m_targetChanged = false;
    m_noGravity     = true;
    m_boundingBox   = {glm::dvec3(-0.25, 0.0, -0.25), glm::dvec3(0.25, 0.5, 0.25)};
}

ExperienceOrb::~ExperienceOrb()
{}

eENTITY_TYPE ExperienceOrb::GetType()
{
    return s_type;
}

bool ExperienceOrb::IsPickable() const
{
    return false;
}

void ExperienceOrb::Tick()
{
    Random &random = m_level->GetRandom();

    m_totalTickCount++;
    UpdateAABB();
    if (IsInWater())
    {
        ApplyWaterFlow();
    }

    if (GetLiquidDepth(false) > 0.1)
    {
        FloatInLiquid(0.99);
    }
    else if (GetLiquidDepth(true) > 0.1)
    {
        FloatInLiquid(0.95);
    }
    else
    {
        m_velocity.y -= 0.03;
    }
    if (IsInLava())
    {
        m_velocity = glm::dvec3((random.NextFloat() - random.NextFloat()) * 0.2, 0.2, (random.NextFloat() - random.NextFloat()) * 0.2);
        if (!m_level->IsClientSide())
        {
            m_level->PlaySound("random.fizz", m_position, 0.4f, 2.0f + random.NextFloat() * 0.4f);
        }
    }

    double maxDistance   = 8.0;
    const Player *target = m_level->FindNearestPlayer(m_position, maxDistance);
    if (target != m_followTarget)
    {
        m_followTarget  = target;
        m_targetChanged = true;
    }

    if (target)
    {
        glm::dvec3 delta = (target->GetPosition() + glm::dvec3(0.0, 0.12, 0.0) - m_position) * (1.0 / maxDistance);
        double dist      = glm::length(delta);
        double power     = 1.0 - dist;
        if (power > 0.0)
        {
            m_velocity += delta * (power * power * 0.1 / dist);
        }
    }

    Move(m_velocity.x, m_velocity.y, m_velocity.z);
    double friction = m_onGround ? GetGroundSlipperiness() * 0.98 : 0.98;
    m_velocity.x *= friction;
    m_velocity.y *= 0.98;
    m_velocity.z *= friction;
    if (m_onGround)
    {
        m_velocity.y *= -0.9;
    }

    m_age++;
    if (m_age >= s_lifetime)
    {
        m_level->RemoveEntity(this);
    }
}

int ExperienceOrb::GetUpdateInterval() const
{
    return 20;
}

bool ExperienceOrb::SendsVelocity() const
{
    return true;
}

void ExperienceOrb::ReadSpawnData(int data)
{
    m_value = data;
}

int ExperienceOrb::GetSpawnData()
{
    return m_value;
}

void ExperienceOrb::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    if (tag->Contains("Value"))
    {
        m_value = tag->GetShort("Value");
    }

    if (tag->Contains("Age"))
    {
        m_age = tag->GetShort("Age");
    }
}

void ExperienceOrb::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    tag->PutShort("Value", (int16_t) m_value);
    tag->PutShort("Age", (int16_t) m_age);
}

int ExperienceOrb::GetValue() const
{
    return m_value;
}

int ExperienceOrb::GetIcon() const
{
    for (int i = 0; i < (int) std::size(kValueTiers); i++)
    {
        if (m_value >= kValueTiers[i])
        {
            return (int) std::size(kValueTiers) - i;
        }
    }

    return 0;
}

bool ExperienceOrb::TakeTargetChanged()
{
    bool changed    = m_targetChanged;
    m_targetChanged = false;
    return changed;
}

int ExperienceOrb::GetExperienceValue(int maxValue)
{
    for (size_t i = 0; i < std::size(kValueTiers); i++)
    {
        if (maxValue >= kValueTiers[i])
        {
            return kValueTiers[i];
        }
    }

    return 1;
}

void ExperienceOrb::SpawnOrbs(Level *level, const glm::dvec3 &position, int amount)
{
    Random &random = level->GetRandom();

    while (amount > 0)
    {
        int value = GetExperienceValue(amount);
        amount -= value;
        std::unique_ptr<ExperienceOrb> orb(new ExperienceOrb(level));
        orb->m_value = value;
        orb->SetPosition(position);
        orb->SetVelocity(glm::dvec3((random.NextFloat() * 0.2 - 0.1) * 2.0, random.NextFloat() * 0.2 * 2.0, (random.NextFloat() * 0.2 - 0.1) * 2.0));
        level->AddEntity(std::move(orb));
    }
}
