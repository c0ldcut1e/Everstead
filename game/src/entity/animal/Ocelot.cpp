#include "Ocelot.h"

#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/PanicGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"

Ocelot::Ocelot(Level *level) : Animal(level)
{
    SetSize(0.6f, 0.8f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(10.0);
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.3);
    SetHealth(10);
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(1, new PanicGoal(this, 1.33));
    m_goalSelector.AddGoal(5, new RandomStrollGoal(this, 0.8));
    m_goalSelector.AddGoal(6, new LookAtPlayerGoal(this, 10.0f));
    m_goalSelector.AddGoal(7, new RandomLookAroundGoal(this));
}

eENTITY_TYPE Ocelot::GetType()
{
    return s_type;
}

std::unique_ptr<AgeableMob> Ocelot::CreateOffspring(AgeableMob *partner)
{
    (void) partner;
    return std::make_unique<Ocelot>(m_level);
}

void Ocelot::CauseFallDamage(float distance)
{
    (void) distance;
}

std::string Ocelot::GetAmbientSound() const
{
    return "mob.ocelot.idle" + std::to_string(GetRandom().NextInt(1, 4));
}

std::string Ocelot::GetHurtSound() const
{
    return "mob.ocelot.hurt" + std::to_string(GetRandom().NextInt(1, 3));
}

std::string Ocelot::GetDeathSound() const
{
    return "mob.ocelot.death" + std::to_string(GetRandom().NextInt(1, 3));
}
