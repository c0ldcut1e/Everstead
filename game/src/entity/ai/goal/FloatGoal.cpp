#include "FloatGoal.h"

#include "../../Mob.h"
#include "../control/JumpControl.h"
#include "../pathfinder/PathNavigation.h"

FloatGoal::FloatGoal(Mob *mob)
{
    m_mob   = mob;
    m_flags = eGoalFlag_Jump;
    mob->GetNavigation()->SetCanFloat(true);
}

bool FloatGoal::CanUse()
{
    return m_mob->IsInWater() || m_mob->IsInLava();
}

void FloatGoal::Tick()
{
    if (m_mob->GetRandom().NextFloat() < 0.8f)
    {
        m_mob->GetJumpControl()->Jump();
    }
}
