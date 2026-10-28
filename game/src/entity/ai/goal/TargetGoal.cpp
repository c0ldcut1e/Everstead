#include "TargetGoal.h"

#include "../../Mob.h"

#include <glm/gtx/norm.hpp>

TargetGoal::TargetGoal(Mob *mob, double within, bool mustSee)
{
    m_mob         = mob;
    m_within      = within;
    m_mustSee     = mustSee;
    m_unseenTicks = 0;
    m_flags       = eGoalFlag_Target;
}

bool TargetGoal::CanContinueToUse()
{
    LivingEntity *target = m_mob->GetTarget();
    if (!target || !m_mob->CanAttack(target) || glm::distance2(target->GetPosition(), m_mob->GetPosition()) > m_within * m_within)
    {
        return false;
    }

    if (m_mustSee)
    {
        if (m_mob->CanSee(target))
        {
            m_unseenTicks = 0;
        }
        else if (++m_unseenTicks > 60)
        {
            return false;
        }
    }

    return true;
}

void TargetGoal::Start()
{
    m_unseenTicks = 0;
}

void TargetGoal::Stop()
{
    m_mob->SetTarget(nullptr);
}
