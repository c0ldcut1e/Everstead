#include "RefractionFracture.h"

#include <algorithm>

#include "../../entity/player/ServerPlayer.h"
#include "../../entity/projectile/Arrow.h"
#include "../Level.h"
#include "FractureTraits.h"

RefractionFracture::RefractionFracture() : FractureTrait("refraction", FractureTraits::eGearType_Armor, 1)
{}

bool RefractionFracture::OnArrowHit(ServerPlayer *player, Arrow *arrow, int pieces)
{
    if (player->GetLevel()->GetRandom().NextFloat() >= 0.1f * pieces)
    {
        return false;
    }

    Level *level         = player->GetLevel();
    Entity *owner        = level->GetEntityById(arrow->GetOwnerId());
    glm::dvec3 direction = -arrow->GetVelocity();
    if (owner)
    {
        const AABB &box = owner->GetAABB();
        direction       = (box.GetMin() + box.GetMax()) * 0.5 - arrow->GetPosition();
    }

    arrow->Redirect(player, glm::normalize(direction) * std::max(glm::length(arrow->GetVelocity()), 1.0));
    SendParticles(level, eParticleType_RiftSpark, arrow->GetPosition(), glm::dvec3(0.2), 0.2f, 10);
    PlaySound(level, arrow->GetPosition(), 1.0f);
    return true;
}
