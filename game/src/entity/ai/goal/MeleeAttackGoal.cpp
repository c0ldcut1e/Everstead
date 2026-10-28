#include "MeleeAttackGoal.h"

#include "../../Mob.h"
#include "../control/LookControl.h"
#include "../pathfinder/PathNavigation.h"

#include <glm/gtx/norm.hpp>

MeleeAttackGoal::MeleeAttackGoal(Mob *mob, double speed, bool trackTarget)
{
    m_mob              = mob;
    m_speed            = speed;
    m_trackTarget      = trackTarget;
    m_timeToRecalcPath = 0;
    m_flags            = eGoalFlag_Move | eGoalFlag_Look;
}

bool MeleeAttackGoal::CanUse()
{
    LivingEntity *target = m_mob->GetTarget();
    if (!target || target->GetHealth() <= 0 || --m_timeToRecalcPath > 0)
    {
        return false;
    }

    // An unreachable target makes the pathfinder search its whole range, so we wait a second before trying again
    m_timeToRecalcPath = 20;
    return m_mob->GetNavigation()->MoveTo(target, m_speed);
}

bool MeleeAttackGoal::CanContinueToUse()
{
    LivingEntity *target = m_mob->GetTarget();
    return target && target->GetHealth() > 0 && (m_trackTarget || !m_mob->GetNavigation()->IsDone());
}

void MeleeAttackGoal::Start()
{
    m_mob->GetNavigation()->MoveTo(m_mob->GetTarget(), m_speed);
    m_timeToRecalcPath = 0;
}

void MeleeAttackGoal::Stop()
{
    m_mob->GetNavigation()->Stop();
}

void MeleeAttackGoal::Tick()
{
    LivingEntity *target = m_mob->GetTarget();
    m_mob->GetLookControl()->SetLookAt(target, 30.0f, 30.0f);
    if ((m_trackTarget || m_mob->CanSee(target)) && --m_timeToRecalcPath <= 0)
    {
        m_timeToRecalcPath = 4 + m_mob->GetRandom().NextInt(0, 6);
        m_mob->GetNavigation()->MoveTo(target, m_speed);
    }

    double reach = m_mob->GetWidth() * 2.0 * m_mob->GetWidth() * 2.0 + target->GetBoundingBox().GetMax().x - target->GetBoundingBox().GetMin().x;
    glm::dvec3 targetFeet(target->GetPosition().x, target->GetAABB().GetMin().y, target->GetPosition().z);
    if (m_mob->GetAttackTime() > 0 || glm::distance2(m_mob->GetPosition(), targetFeet) > reach)
    {
        return;
    }

    m_mob->SetAttackTime(20);
    m_mob->DoHurtTarget(target);
}
