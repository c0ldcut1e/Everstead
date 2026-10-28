#include "RandomStrollGoal.h"

#include "../../Mob.h"
#include "../pathfinder/PathNavigation.h"

RandomStrollGoal::RandomStrollGoal(Mob *mob, double speed)
{
    m_mob     = mob;
    m_speed   = speed;
    m_wantedX = 0.0;
    m_wantedY = 0.0;
    m_wantedZ = 0.0;
    m_flags   = eGoalFlag_Move | eGoalFlag_Look;
}

bool RandomStrollGoal::CanUse()
{
    if (m_mob->GetNoActionTime() >= 100 || m_mob->GetRandom().NextInt(0, 119) != 0)
    {
        return false;
    }

    glm::dvec3 pos;
    if (!m_mob->GetRandomPos(10, 7, &pos))
    {
        return false;
    }

    m_wantedX = pos.x;
    m_wantedY = pos.y;
    m_wantedZ = pos.z;
    return true;
}

bool RandomStrollGoal::CanContinueToUse()
{
    return !m_mob->GetNavigation()->IsDone();
}

void RandomStrollGoal::Start()
{
    m_mob->GetNavigation()->MoveTo(m_wantedX, m_wantedY, m_wantedZ, m_speed);
}
