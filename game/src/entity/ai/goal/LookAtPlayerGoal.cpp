#include "LookAtPlayerGoal.h"

#include "../../Mob.h"
#include "../../player/Player.h"
#include "../control/LookControl.h"

#include <glm/gtx/norm.hpp>

LookAtPlayerGoal::LookAtPlayerGoal(Mob *mob, float lookDistance)
{
    m_mob          = mob;
    m_lookDistance = lookDistance;
    m_lookAtId     = -1;
    m_lookTime     = 0;
    m_flags        = eGoalFlag_Look;
}

bool LookAtPlayerGoal::CanUse()
{
    if (m_mob->GetRandom().NextFloat() >= 0.02f)
    {
        return false;
    }

    Player *player = m_mob->GetLevel()->FindNearestPlayer(m_mob->GetPosition(), m_lookDistance);
    if (!player)
    {
        return false;
    }

    m_lookAtId = player->GetId();
    return true;
}

bool LookAtPlayerGoal::CanContinueToUse()
{
    Player *player = dynamic_cast<Player *>(m_mob->GetLevel()->GetEntityById(m_lookAtId));
    if (!player || player->GetHealth() <= 0)
    {
        return false;
    }

    if (glm::distance2(m_mob->GetPosition(), player->GetPosition()) > (double) (m_lookDistance * m_lookDistance))
    {
        return false;
    }

    return m_lookTime > 0;
}

void LookAtPlayerGoal::Start()
{
    m_lookTime = 40 + m_mob->GetRandom().NextInt(0, 39);
}

void LookAtPlayerGoal::Stop()
{
    m_lookAtId = -1;
}

void LookAtPlayerGoal::Tick()
{
    m_mob->GetLookControl()->SetLookAt(m_mob->GetLevel()->GetEntityById(m_lookAtId), 10.0f, Mob::kMaxHeadPitch);
    m_lookTime--;
}
