#include "Zombie.h"

#include <cmath>

#include "../animal/Chicken.h"

#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/HurtByTargetGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/MeleeAttackGoal.h"
#include "../ai/goal/NearestAttackableTargetGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"

Zombie::Zombie(Level *level) : Monster(level)
{
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.23);
    m_attributes.Get(Attribute::s_attackDamage)->SetBaseValue(4.0);
    m_attributes.Get(Attribute::s_armor)->SetBaseValue(2.0);
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(2, new MeleeAttackGoal(this, 1.0, false));
    m_goalSelector.AddGoal(6, new RandomStrollGoal(this, 1.0));
    m_goalSelector.AddGoal(7, new LookAtPlayerGoal(this, 8.0f));
    m_goalSelector.AddGoal(7, new RandomLookAroundGoal(this));
    m_targetSelector.AddGoal(1, new HurtByTargetGoal(this));
    m_targetSelector.AddGoal(2, new NearestAttackableTargetGoal(this, 16.0, true));
    m_baby = false;
}

eENTITY_TYPE Zombie::GetType()
{
    return s_type;
}

void Zombie::FinalizeSpawn()
{
    if (GetRandom().NextFloat() >= 0.05f)
    {
        return;
    }

    SetBaby(true);
    m_level->OnMobDataChanged(this);
    if (GetRandom().NextFloat() >= 0.05f)
    {
        return;
    }

    Chicken *mount                                       = nullptr;
    const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Chicken *chicken = dynamic_cast<Chicken *>(entities[i].get());
        if (chicken && !chicken->IsBaby() && chicken->GetPassengers().empty() && fabs(chicken->GetPosition().x - m_position.x) <= 5.0 &&
            fabs(chicken->GetPosition().z - m_position.z) <= 5.0 && fabs(chicken->GetPosition().y - m_position.y) <= 3.0)
        {
            mount = chicken;
            break;
        }
    }

    if (!mount && GetRandom().NextFloat() < 0.05f)
    {
        std::unique_ptr<Chicken> chicken = std::make_unique<Chicken>(m_level);
        chicken->SetPosition(m_position);
        chicken->SetRotation(m_yaw, 0.0f);
        mount = chicken.get();
        m_level->AddEntity(std::move(chicken));
    }

    if (mount)
    {
        mount->SetChickenJockey(true);
        StartRiding(mount);
        m_level->OnPassengersChanged(mount);
    }
}

bool Zombie::IsBaby() const
{
    return m_baby;
}

void Zombie::SetBaby(bool baby)
{
    if (m_baby == baby)
    {
        return;
    }

    m_baby = baby;
    if (baby)
    {
        SetSize(0.3f, 0.9f);
        m_attributes.Get(Attribute::s_movementSpeed)->AddModifier(AttributeModifier{"baby_speed_boost", 0.5, eAttributeOperation_AddMultipliedBase});
    }
    else
    {
        SetSize(0.6f, 1.8f);
        m_attributes.Get(Attribute::s_movementSpeed)->RemoveModifier("baby_speed_boost");
    }
}

void Zombie::Load(const CompoundTag *tag)
{
    Monster::Load(tag);
    SetBaby(tag->GetBoolean("IsBaby"));
}

void Zombie::Save(CompoundTag *tag) const
{
    Monster::Save(tag);
    tag->PutBoolean("IsBaby", m_baby);
}

void Zombie::GetMobData(MobData *data) const
{
    Monster::GetMobData(data);
    data->baby = m_baby;
}

void Zombie::ApplyMobData(const MobData &data)
{
    Monster::ApplyMobData(data);
    SetBaby(data.baby);
}

bool Zombie::CanDropLoot() const
{
    return true;
}

int Zombie::GetExperienceReward()
{
    return m_baby ? 12 : Monster::GetExperienceReward();
}

void Zombie::ServerTick()
{
    if (m_health > 0 && !m_baby && m_level->GetSkyDarken() < 4)
    {
        float brightness = GetBrightness();
        if (brightness > 0.5f && GetRandom().NextFloat() * 30.0f < (brightness - 0.4f) * 2.0f &&
            (int) floor(m_position.y) >= m_level->GetRainHeight((int) floor(m_position.x), (int) floor(m_position.z)))
        {
            SetOnFire(8);
        }
    }

    Monster::ServerTick();
}

std::string Zombie::GetAmbientSound() const
{
    return "mob.zombie.say" + std::to_string(GetRandom().NextInt(1, 3));
}

std::string Zombie::GetHurtSound() const
{
    return "mob.zombie.hurt" + std::to_string(GetRandom().NextInt(1, 2));
}

std::string Zombie::GetDeathSound() const
{
    return "mob.zombie.death";
}
