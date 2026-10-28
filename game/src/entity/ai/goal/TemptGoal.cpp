#include "TemptGoal.h"

#include "../../Mob.h"
#include "../../player/Player.h"
#include "../control/LookControl.h"
#include "../pathfinder/PathNavigation.h"

#include <glm/gtx/norm.hpp>

TemptGoal::TemptGoal(Mob *mob, double speed, Item *item)
{
    m_mob           = mob;
    m_speed         = speed;
    m_item          = item;
    m_playerId      = -1;
    m_calmDown      = 0;
    m_oldAvoidWater = false;
    m_flags         = eGoalFlag_Move | eGoalFlag_Look;
}

bool TemptGoal::CanUse()
{
    if (m_calmDown > 0)
    {
        m_calmDown--;
        return false;
    }

    Player *player = m_mob->GetLevel()->FindNearestPlayer(m_mob->GetPosition(), 10.0);
    if (!player)
    {
        return false;
    }

    ItemInstance *held = player->GetInventory()->GetSelectedItem();
    if (held->IsEmpty() || held->GetItem() != m_item)
    {
        return false;
    }

    m_playerId = player->GetId();
    return true;
}

void TemptGoal::Start()
{
    m_oldAvoidWater = m_mob->GetNavigation()->GetAvoidWater();
    m_mob->GetNavigation()->SetAvoidWater(false);
}

void TemptGoal::Stop()
{
    m_playerId = -1;
    m_mob->GetNavigation()->Stop();
    m_calmDown = 100;
    m_mob->GetNavigation()->SetAvoidWater(m_oldAvoidWater);
}

void TemptGoal::Tick()
{
    Entity *player = m_mob->GetLevel()->GetEntityById(m_playerId);
    m_mob->GetLookControl()->SetLookAt(player, 30.0f, Mob::kMaxHeadPitch);
    if (glm::distance2(m_mob->GetPosition(), player->GetPosition()) < 2.5 * 2.5)
    {
        m_mob->GetNavigation()->Stop();
    }
    else
    {
        m_mob->GetNavigation()->MoveTo(player, m_speed);
    }
}
