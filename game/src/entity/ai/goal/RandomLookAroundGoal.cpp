#include "RandomLookAroundGoal.h"

#include <cmath>
#include <numbers>

#include "../../Mob.h"
#include "../control/LookControl.h"

RandomLookAroundGoal::RandomLookAroundGoal(Mob *mob)
{
    m_mob      = mob;
    m_relX     = 0.0;
    m_relZ     = 0.0;
    m_lookTime = 0;
    m_flags    = eGoalFlag_Move | eGoalFlag_Look;
}

bool RandomLookAroundGoal::CanUse()
{
    return m_mob->GetRandom().NextFloat() < 0.02f;
}

bool RandomLookAroundGoal::CanContinueToUse()
{
    return m_lookTime >= 0;
}

void RandomLookAroundGoal::Start()
{
    double angle = std::numbers::pi * 2.0 * m_mob->GetRandom().NextDouble();
    m_relX       = cos(angle);
    m_relZ       = sin(angle);
    m_lookTime   = 20 + m_mob->GetRandom().NextInt(0, 19);
}

void RandomLookAroundGoal::Tick()
{
    m_lookTime--;
    const glm::dvec3 &pos = m_mob->GetPosition();
    m_mob->GetLookControl()->SetLookAt(pos.x + m_relX, pos.y + m_mob->GetEyeHeight(), pos.z + m_relZ, 10.0f, Mob::kMaxHeadPitch);
}
