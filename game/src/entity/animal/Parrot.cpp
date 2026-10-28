#include "Parrot.h"

#include <algorithm>

#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/PanicGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"

Parrot::Parrot(Level *level) : Animal(level)
{
    SetSize(0.5f, 0.9f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(6.0);
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.2);
    SetHealth(6);
    m_flap         = 0.0f;
    m_oldFlap      = 0.0f;
    m_flapSpeed    = 0.0f;
    m_oldFlapSpeed = 0.0f;
    m_flapping     = 1.0f;
    m_variant      = GetRandom().NextInt(0, kVariantCount - 1);
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(1, new PanicGoal(this, 1.25));
    m_goalSelector.AddGoal(5, new RandomStrollGoal(this, 1.0));
    m_goalSelector.AddGoal(6, new LookAtPlayerGoal(this, 8.0f));
    m_goalSelector.AddGoal(7, new RandomLookAroundGoal(this));
}

eENTITY_TYPE Parrot::GetType()
{
    return s_type;
}

std::unique_ptr<AgeableMob> Parrot::CreateOffspring(AgeableMob *partner)
{
    (void) partner;
    return std::make_unique<Parrot>(m_level);
}

void Parrot::Load(const CompoundTag *tag)
{
    Animal::Load(tag);
    m_variant = std::clamp(tag->GetInt("Variant"), 0, kVariantCount - 1);
}

void Parrot::Save(CompoundTag *tag) const
{
    Animal::Save(tag);
    tag->PutInt("Variant", m_variant);
}

void Parrot::GetMobData(MobData *data) const
{
    Animal::GetMobData(data);
    data->variant = (uint8_t) m_variant;
}

void Parrot::ApplyMobData(const MobData &data)
{
    Animal::ApplyMobData(data);
    m_variant = std::min((int) data.variant, kVariantCount - 1);
}

int Parrot::GetVariant() const
{
    return m_variant;
}

float Parrot::GetFlap(float partialTicks) const
{
    return m_oldFlap + (m_flap - m_oldFlap) * partialTicks;
}

float Parrot::GetFlapSpeed(float partialTicks) const
{
    return m_oldFlapSpeed + (m_flapSpeed - m_oldFlapSpeed) * partialTicks;
}

void Parrot::ServerTick()
{
    Animal::ServerTick();
    if (!m_onGround && m_velocity.y < 0.0)
    {
        m_velocity.y *= 0.6;
    }
}

void Parrot::ClientTick()
{
    Animal::ClientTick();
    m_oldFlap      = m_flap;
    m_oldFlapSpeed = m_flapSpeed;
    m_flapSpeed    = std::clamp(m_flapSpeed + (m_grounded ? -1.0f : 4.0f) * 0.3f, 0.0f, 1.0f);
    if (!m_grounded && m_flapping < 1.0f)
    {
        m_flapping = 1.0f;
    }

    m_flapping *= 0.9f;
    m_flap += m_flapping * 2.0f;
}

void Parrot::CauseFallDamage(float distance)
{
    (void) distance;
}

std::string Parrot::GetAmbientSound() const
{
    return "mob.parrot.idle" + std::to_string(GetRandom().NextInt(1, 6));
}

std::string Parrot::GetHurtSound() const
{
    return "mob.parrot.hurt" + std::to_string(GetRandom().NextInt(1, 2));
}

std::string Parrot::GetDeathSound() const
{
    return "mob.parrot.death" + std::to_string(GetRandom().NextInt(1, 4));
}
