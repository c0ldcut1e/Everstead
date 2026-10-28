#include "NearestAttackableTargetGoal.h"

#include "../../Mob.h"
#include "../../player/Player.h"

#include <glm/gtx/norm.hpp>

NearestAttackableTargetGoal::NearestAttackableTargetGoal(Mob *mob, double within, bool mustSee) : TargetGoal(mob, within, mustSee)
{
    m_targetId = -1;
}

bool NearestAttackableTargetGoal::CanUse()
{
    Player *player = m_mob->GetLevel()->FindNearestPlayer(m_mob->GetPosition(), m_within);
    if (!player || !m_mob->CanAttack(player) || (m_mustSee && !m_mob->CanSee(player)))
    {
        return false;
    }

    if (player->IsSneaking() && glm::distance2(player->GetPosition(), m_mob->GetPosition()) > m_within * m_within * 0.64)
    {
        return false;
    }

    m_targetId = player->GetId();
    return true;
}

void NearestAttackableTargetGoal::Start()
{
    m_mob->SetTarget(dynamic_cast<LivingEntity *>(m_mob->GetLevel()->GetEntityById(m_targetId)));
    TargetGoal::Start();
}
