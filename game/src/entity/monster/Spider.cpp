#include "Spider.h"

#include <cmath>

#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/HurtByTargetGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/MeleeAttackGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"
#include "../player/Player.h"
#include "Skeleton.h"

#include <glm/gtx/norm.hpp>

Spider::Spider(Level *level) : Monster(level)
{
    SetSize(1.4f, 0.9f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(16.0);
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.3);
    SetHealth(16);
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(2, new MeleeAttackGoal(this, 1.0, true));
    m_goalSelector.AddGoal(5, new RandomStrollGoal(this, 0.8));
    m_goalSelector.AddGoal(6, new LookAtPlayerGoal(this, 8.0f));
    m_goalSelector.AddGoal(6, new RandomLookAroundGoal(this));
    m_targetSelector.AddGoal(1, new HurtByTargetGoal(this));
}

eENTITY_TYPE Spider::GetType()
{
    return s_type;
}

bool Spider::IsOnLadder() const
{
    return m_horizontalCollision;
}

void Spider::FinalizeSpawn()
{
    if (GetRandom().NextInt(0, 99) != 0)
    {
        return;
    }

    std::unique_ptr<Skeleton> skeleton = std::make_unique<Skeleton>(m_level);
    skeleton->SetPosition(m_position);
    skeleton->SetRotation(m_yaw, 0.0f);
    Skeleton *rider = skeleton.get();
    m_level->AddEntity(std::move(skeleton));
    rider->StartRiding(this);
    m_level->OnPassengersChanged(this);
}

double Spider::GetRideHeight() const
{
    return m_height * 0.75 - 0.5;
}

// We target here instead of with target goals because spiders pick and drop targets based on light
void Spider::ServerTick()
{
    LivingEntity *target = GetTarget();
    if (m_health > 0 && !target)
    {
        Player *player = m_level->FindNearestPlayer(m_position, 16.0);
        if (GetBrightness() < 0.5f && player && CanAttack(player) &&
            (!player->IsSneaking() || glm::distance2(player->GetPosition(), m_position) <= 16.0 * 16.0 * 0.64))
        {
            SetTarget(player);
        }
    }
    else if (m_health > 0 && (!CanAttack(target) || (GetBrightness() > 0.5f && GetRandom().NextInt(0, 99) == 0)))
    {
        SetTarget(nullptr);
    }
    else if (m_health > 0)
    {
        double deltaX   = target->GetPosition().x - m_position.x;
        double deltaZ   = target->GetPosition().z - m_position.z;
        double distance = sqrt(glm::distance2(target->GetPosition(), m_position));
        if (distance > 2.0 && distance < 6.0 && GetRandom().NextInt(0, 9) == 0 && m_onGround && CanSee(target))
        {
            double horizontal = sqrt(deltaX * deltaX + deltaZ * deltaZ);
            m_velocity = glm::dvec3(deltaX / horizontal * 0.5 * 0.8 + m_velocity.x * 0.2, 0.4, deltaZ / horizontal * 0.5 * 0.8 + m_velocity.z * 0.2);
        }
    }

    Monster::ServerTick();
}

std::string Spider::GetAmbientSound() const
{
    return "mob.spider.say" + std::to_string(GetRandom().NextInt(1, 4));
}

std::string Spider::GetHurtSound() const
{
    return GetAmbientSound();
}

std::string Spider::GetDeathSound() const
{
    return "mob.spider.death";
}
