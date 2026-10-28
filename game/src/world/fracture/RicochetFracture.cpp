#include "RicochetFracture.h"

#include "../../entity/monster/Monster.h"
#include "../../entity/player/Player.h"
#include "../../entity/projectile/Arrow.h"
#include "../../utils/Direction.h"
#include "../Level.h"
#include "FractureTraits.h"

RicochetFracture::RicochetFracture() : FractureTrait("ricochet", FractureTraits::eGearType_Bow, 3)
{}

bool RicochetFracture::OnArrowLand(Arrow *arrow, Direction *face)
{
    if (arrow->GetHomingTargetId() != 0 || arrow->GetLevel()->GetRandom().NextFloat() >= 0.3f)
    {
        return false;
    }

    Level *level                                         = arrow->GetLevel();
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    LivingEntity *target                                 = nullptr;
    double nearest                                       = 16.0 * 16.0;
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        if (entity->GetId() == arrow->GetOwnerId() || (!dynamic_cast<Monster *>(entity) && !dynamic_cast<Player *>(entity)))
        {
            continue;
        }

        LivingEntity *living = (LivingEntity *) entity;
        glm::dvec3 delta     = living->GetPosition() - arrow->GetPosition();
        double distance      = glm::dot(delta, delta);
        if (living->GetHealth() > 0 && distance < nearest)
        {
            target  = living;
            nearest = distance;
        }
    }

    if (!target)
    {
        return false;
    }

    glm::dvec3 velocity = arrow->GetVelocity();
    if (face->dx != 0)
    {
        velocity.x = -velocity.x;
    }

    if (face->dy != 0)
    {
        velocity.y = -velocity.y;
    }

    if (face->dz != 0)
    {
        velocity.z = -velocity.z;
    }

    arrow->HomeOn(target, velocity * 0.8);
    SendParticles(level, eParticleType_RiftSpark, arrow->GetPosition(), glm::dvec3(0.15), 0.2f, 8);
    PlaySound(level, arrow->GetPosition(), 0.8f);
    return true;
}
