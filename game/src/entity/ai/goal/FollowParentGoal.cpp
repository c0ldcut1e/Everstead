#include "FollowParentGoal.h"

#include <vector>

#include "../../AgeableMob.h"
#include "../pathfinder/PathNavigation.h"

#include <glm/gtx/norm.hpp>

FollowParentGoal::FollowParentGoal(AgeableMob *mob, double speed)
{
    m_mob              = mob;
    m_speed            = speed;
    m_parentId         = -1;
    m_timeToRecalcPath = 0;
}

bool FollowParentGoal::CanUse()
{
    if (m_mob->GetAge() >= 0)
    {
        return false;
    }

    const AABB &box = m_mob->GetAABB();
    glm::dvec3 grow(8.0, 4.0, 8.0);
    AABB area(box.GetMin() - grow, box.GetMax() + grow);
    AgeableMob *closest                                  = nullptr;
    double closestDist                                   = 0.0;
    const std::vector<std::unique_ptr<Entity>> &entities = m_mob->GetLevel()->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        if (entity->GetType() != m_mob->GetType() || !entity->GetAABB().Intersects(area))
        {
            continue;
        }

        AgeableMob *parent = (AgeableMob *) entity;
        if (parent->GetAge() < 0)
        {
            continue;
        }

        double dist = glm::distance2(m_mob->GetPosition(), parent->GetPosition());
        if (!closest || dist < closestDist)
        {
            closest     = parent;
            closestDist = dist;
        }
    }

    if (!closest || closestDist < 9.0)
    {
        return false;
    }

    m_parentId = closest->GetId();
    return true;
}

bool FollowParentGoal::CanContinueToUse()
{
    AgeableMob *parent = (AgeableMob *) m_mob->GetLevel()->GetEntityById(m_parentId);
    if (!parent || parent->GetHealth() <= 0)
    {
        return false;
    }

    double dist = glm::distance2(m_mob->GetPosition(), parent->GetPosition());
    return dist >= 9.0 && dist <= 256.0;
}

void FollowParentGoal::Start()
{
    m_timeToRecalcPath = 0;
}

void FollowParentGoal::Stop()
{
    m_parentId = -1;
}

void FollowParentGoal::Tick()
{
    if (--m_timeToRecalcPath > 0)
    {
        return;
    }

    m_timeToRecalcPath = 10;
    m_mob->GetNavigation()->MoveTo(m_mob->GetLevel()->GetEntityById(m_parentId), m_speed);
}
