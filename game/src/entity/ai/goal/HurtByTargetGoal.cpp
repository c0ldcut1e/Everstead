#include "HurtByTargetGoal.h"

#include "../../Mob.h"

HurtByTargetGoal::HurtByTargetGoal(Mob *mob) : TargetGoal(mob, 16.0, false)
{
    m_timestamp = 0;
}

bool HurtByTargetGoal::CanUse()
{
    LivingEntity *attacker = m_mob->GetLastHurtByMob();
    return attacker && m_mob->GetLastHurtByMobTimestamp() != m_timestamp && m_mob->CanAttack(attacker);
}

void HurtByTargetGoal::Start()
{
    m_mob->SetTarget(m_mob->GetLastHurtByMob());
    m_timestamp = m_mob->GetLastHurtByMobTimestamp();
    TargetGoal::Start();
}
