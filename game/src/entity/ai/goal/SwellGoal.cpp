#include "SwellGoal.h"

#include "../../monster/Creeper.h"
#include "../pathfinder/PathNavigation.h"

#include <glm/gtx/norm.hpp>

SwellGoal::SwellGoal(Creeper *creeper)
{
    m_creeper = creeper;
    m_flags   = eGoalFlag_Move;
}

bool SwellGoal::CanUse()
{
    LivingEntity *target = m_creeper->GetTarget();
    return m_creeper->GetSwellDir() > 0 || (target && glm::distance2(m_creeper->GetPosition(), target->GetPosition()) < 9.0);
}

void SwellGoal::Start()
{
    m_creeper->GetNavigation()->Stop();
}

void SwellGoal::Tick()
{
    LivingEntity *target = m_creeper->GetTarget();
    bool swell           = target && glm::distance2(m_creeper->GetPosition(), target->GetPosition()) <= 49.0 && m_creeper->CanSee(target);
    m_creeper->SetSwellDir(swell ? 1 : -1);
}
