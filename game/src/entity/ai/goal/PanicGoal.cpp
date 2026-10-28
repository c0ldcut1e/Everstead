#include "PanicGoal.h"

#include "../../Mob.h"
#include "../pathfinder/PathNavigation.h"

PanicGoal::PanicGoal(Mob *mob, double speed)
{
    m_mob   = mob;
    m_speed = speed;
    m_posX  = 0.0;
    m_posY  = 0.0;
    m_posZ  = 0.0;
    m_flags = eGoalFlag_Move;
}

bool PanicGoal::CanUse()
{
    glm::dvec3 pos;
    if (!m_mob->GetLastHurtByMob() || !m_mob->GetRandomPos(5, 4, &pos))
    {
        return false;
    }

    m_posX = pos.x;
    m_posY = pos.y;
    m_posZ = pos.z;
    return true;
}

bool PanicGoal::CanContinueToUse()
{
    return !m_mob->GetNavigation()->IsDone();
}

void PanicGoal::Start()
{
    m_mob->GetNavigation()->MoveTo(m_posX, m_posY, m_posZ, m_speed);
}
