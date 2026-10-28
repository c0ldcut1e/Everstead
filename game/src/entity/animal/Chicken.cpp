#include "Chicken.h"

#include <algorithm>

#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../ai/goal/BreedGoal.h"
#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/FollowParentGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/PanicGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"
#include "../ai/goal/TemptGoal.h"

Chicken::Chicken(Level *level) : Animal(level)
{
    SetSize(0.3f, 0.7f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(4.0);
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.25);
    SetHealth(4);
    m_flap          = 0.0f;
    m_oldFlap       = 0.0f;
    m_flapSpeed     = 0.0f;
    m_oldFlapSpeed  = 0.0f;
    m_flapping      = 1.0f;
    m_eggTime       = GetRandom().NextInt(6000, 11999);
    m_chickenJockey = false;
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(1, new PanicGoal(this, 1.52));
    m_goalSelector.AddGoal(2, new BreedGoal(this, 1.0));
    m_goalSelector.AddGoal(3, new TemptGoal(this, 1.0, Item::ByName("wheat_seeds")));
    m_goalSelector.AddGoal(4, new FollowParentGoal(this, 1.12));
    m_goalSelector.AddGoal(5, new RandomStrollGoal(this, 1.0));
    m_goalSelector.AddGoal(6, new LookAtPlayerGoal(this, 6.0f));
    m_goalSelector.AddGoal(7, new RandomLookAroundGoal(this));
}

eENTITY_TYPE Chicken::GetType()
{
    return s_type;
}

std::unique_ptr<AgeableMob> Chicken::CreateOffspring(AgeableMob *partner)
{
    (void) partner;
    return std::make_unique<Chicken>(m_level);
}

bool Chicken::IsFood(const ItemInstance &item) const
{
    return item.GetItem() == Item::ByName("wheat_seeds");
}

void Chicken::Load(const CompoundTag *tag)
{
    Animal::Load(tag);
    if (tag->Contains("EggLayTime"))
    {
        m_eggTime = tag->GetInt("EggLayTime");
    }

    m_chickenJockey = tag->GetBoolean("IsChickenJockey");
}

void Chicken::Save(CompoundTag *tag) const
{
    Animal::Save(tag);
    tag->PutInt("EggLayTime", m_eggTime);
    tag->PutBoolean("IsChickenJockey", m_chickenJockey);
}

void Chicken::SetChickenJockey(bool chickenJockey)
{
    m_chickenJockey = chickenJockey;
}

float Chicken::GetFlap(float partialTicks) const
{
    return m_oldFlap + (m_flap - m_oldFlap) * partialTicks;
}

float Chicken::GetFlapSpeed(float partialTicks) const
{
    return m_oldFlapSpeed + (m_flapSpeed - m_oldFlapSpeed) * partialTicks;
}

void Chicken::ServerTick()
{
    Animal::ServerTick();
    if (!m_onGround && m_velocity.y < 0.0)
    {
        m_velocity.y *= 0.6;
    }

    if (m_health > 0 && !IsBaby() && !m_chickenJockey && --m_eggTime <= 0)
    {
        m_level->PlaySound("mob.chicken.plop", m_position, 1.0f, (GetRandom().NextFloat() - GetRandom().NextFloat()) * 0.2f + 1.0f);
        SpawnAtLocation(Item::ByName("egg"), 1);
        m_eggTime = GetRandom().NextInt(6000, 11999);
    }
}

bool Chicken::RemoveWhenFarAway() const
{
    return m_chickenJockey;
}

int Chicken::GetExperienceReward()
{
    return m_chickenJockey ? 10 : Animal::GetExperienceReward();
}

double Chicken::GetRideHeight() const
{
    return m_height * 0.5;
}

void Chicken::ClientTick()
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

void Chicken::CauseFallDamage(float distance)
{
    (void) distance;
}

std::string Chicken::GetAmbientSound() const
{
    return "mob.chicken.say" + std::to_string(GetRandom().NextInt(1, 3));
}

std::string Chicken::GetHurtSound() const
{
    return "mob.chicken.hurt" + std::to_string(GetRandom().NextInt(1, 2));
}

std::string Chicken::GetDeathSound() const
{
    return GetHurtSound();
}
