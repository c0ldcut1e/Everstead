#include "Skeleton.h"

#include <cmath>
#include <memory>

#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/HurtByTargetGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/NearestAttackableTargetGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"
#include "../ai/goal/RangedAttackGoal.h"
#include "../projectile/Arrow.h"

Skeleton::Skeleton(Level *level) : Monster(level)
{
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.25);
    m_goalSelector.AddGoal(1, new FloatGoal(this));
    m_goalSelector.AddGoal(4, new RangedAttackGoal(this, 1.0, 20, 60, 15.0f));
    m_goalSelector.AddGoal(5, new RandomStrollGoal(this, 1.0));
    m_goalSelector.AddGoal(6, new LookAtPlayerGoal(this, 8.0f));
    m_goalSelector.AddGoal(6, new RandomLookAroundGoal(this));
    m_targetSelector.AddGoal(1, new HurtByTargetGoal(this));
    m_targetSelector.AddGoal(2, new NearestAttackableTargetGoal(this, 16.0, true));
}

eENTITY_TYPE Skeleton::GetType()
{
    return s_type;
}

void Skeleton::PerformRangedAttack(LivingEntity *target)
{
    std::unique_ptr<Arrow> arrow(new Arrow(m_level));
    arrow->ShootAt(this, target, 1.6f, (float) (14 - m_level->GetDifficulty() * 4));
    m_level->PlaySound("random.bow", m_position, 1.0f, 1.0f / (GetRandom().NextFloat() * 0.4f + 0.8f));
    m_level->AddEntity(std::move(arrow));
}

void Skeleton::ServerTick()
{
    if (m_health > 0 && m_level->GetSkyDarken() < 4)
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

std::string Skeleton::GetAmbientSound() const
{
    return "mob.skeleton.say" + std::to_string(GetRandom().NextInt(1, 3));
}

std::string Skeleton::GetHurtSound() const
{
    return "mob.skeleton.hurt" + std::to_string(GetRandom().NextInt(1, 4));
}

std::string Skeleton::GetDeathSound() const
{
    return "mob.skeleton.death";
}
