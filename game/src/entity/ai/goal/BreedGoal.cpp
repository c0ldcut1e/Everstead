#include "BreedGoal.h"

#include <memory>
#include <utility>
#include <vector>

#include "../../animal/Animal.h"
#include "../../item/ExperienceOrb.h"
#include "../control/LookControl.h"
#include "../pathfinder/PathNavigation.h"

#include <glm/gtx/norm.hpp>

BreedGoal::BreedGoal(Animal *animal, double speed)
{
    m_animal    = animal;
    m_speed     = speed;
    m_partnerId = -1;
    m_loveTime  = 0;
    m_flags     = eGoalFlag_Move | eGoalFlag_Look;
}

bool BreedGoal::CanUse()
{
    if (!m_animal->IsInLove())
    {
        return false;
    }

    Animal *partner = FindFreePartner();
    if (!partner)
    {
        return false;
    }

    m_partnerId = partner->GetId();
    return true;
}

bool BreedGoal::CanContinueToUse()
{
    Animal *partner = GetPartner();
    return partner && partner->GetHealth() > 0 && partner->IsInLove() && m_loveTime < 60;
}

void BreedGoal::Stop()
{
    m_partnerId = -1;
    m_loveTime  = 0;
}

void BreedGoal::Tick()
{
    Animal *partner = GetPartner();
    m_animal->GetLookControl()->SetLookAt(partner, 10.0f, Mob::kMaxHeadPitch);
    m_animal->GetNavigation()->MoveTo(partner, m_speed);
    m_loveTime++;
    if (m_loveTime == 60)
    {
        Breed(partner);
    }
}

Animal *BreedGoal::GetPartner() const
{
    return (Animal *) m_animal->GetLevel()->GetEntityById(m_partnerId);
}

Animal *BreedGoal::FindFreePartner() const
{
    AABB area                                            = m_animal->GetAABB().Inflated(8.0);
    Animal *closest                                      = nullptr;
    double closestDist                                   = 0.0;
    const std::vector<std::unique_ptr<Entity>> &entities = m_animal->GetLevel()->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        if (entity->GetType() != m_animal->GetType() || !entity->GetAABB().Intersects(area))
        {
            continue;
        }

        Animal *other = (Animal *) entity;
        if (!m_animal->CanMate(other))
        {
            continue;
        }

        double dist = glm::distance2(m_animal->GetPosition(), other->GetPosition());
        if (!closest || dist < closestDist)
        {
            closest     = other;
            closestDist = dist;
        }
    }

    return closest;
}

void BreedGoal::Breed(Animal *partner)
{
    std::unique_ptr<AgeableMob> baby = m_animal->CreateOffspring(partner);
    m_animal->SetAge(6000);
    partner->SetAge(6000);
    m_animal->ResetLove();
    partner->ResetLove();
    baby->SetAge(-24000);
    baby->SetPosition(m_animal->GetPosition());
    baby->SetRotation(m_animal->GetYaw(), m_animal->GetPitch());
    m_animal->GetLevel()->AddEntity(std::move(baby));
    ExperienceOrb::SpawnOrbs(m_animal->GetLevel(), m_animal->GetPosition(), m_animal->GetRandom().NextInt(1, 7));
}
