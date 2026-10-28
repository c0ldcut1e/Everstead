#include "RangedAttackGoal.h"

#include <cmath>

#include "../../monster/Skeleton.h"
#include "../control/LookControl.h"
#include "../pathfinder/PathNavigation.h"

#include <glm/gtx/norm.hpp>

RangedAttackGoal::RangedAttackGoal(Skeleton *mob, double speed, int attackIntervalMin, int attackIntervalMax, float attackRadius)
{
    m_mob               = mob;
    m_speed             = speed;
    m_attackIntervalMin = attackIntervalMin;
    m_attackIntervalMax = attackIntervalMax;
    m_attackRadius      = attackRadius;
    m_attackTime        = -1;
    m_seeTime           = 0;
    m_flags             = eGoalFlag_Move | eGoalFlag_Look;
}

bool RangedAttackGoal::CanUse()
{
    return m_mob->GetTarget();
}

bool RangedAttackGoal::CanContinueToUse()
{
    return CanUse() || !m_mob->GetNavigation()->IsDone();
}

void RangedAttackGoal::Stop()
{
    m_seeTime    = 0;
    m_attackTime = -1;
}

void RangedAttackGoal::Tick()
{
    LivingEntity *target = m_mob->GetTarget();
    if (!target)
    {
        return;
    }

    double distanceSq = glm::distance2(m_mob->GetPosition(), glm::dvec3(target->GetPosition().x, target->GetAABB().GetMin().y, target->GetPosition().z));
    bool canSee       = m_mob->CanSee(target);
    m_seeTime         = canSee ? m_seeTime + 1 : 0;
    if (distanceSq <= m_attackRadius * m_attackRadius && m_seeTime >= 20)
    {
        m_mob->GetNavigation()->Stop();
    }
    else
    {
        m_mob->GetNavigation()->MoveTo(target, m_speed);
    }

    m_mob->GetLookControl()->SetLookAt(target, 30.0f, 30.0f);
    float distance = (float) sqrt(distanceSq) / m_attackRadius;
    if (--m_attackTime == 0)
    {
        if (distanceSq > m_attackRadius * m_attackRadius || !canSee)
        {
            return;
        }

        m_mob->PerformRangedAttack(target);
        m_attackTime = (int) floor(distance * (m_attackIntervalMax - m_attackIntervalMin) + m_attackIntervalMin);
    }
    else if (m_attackTime < 0)
    {
        m_attackTime = (int) floor(distance * (m_attackIntervalMax - m_attackIntervalMin) + m_attackIntervalMin);
    }
}
