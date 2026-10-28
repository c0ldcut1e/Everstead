#include "AfterimageFracture.h"

#include <vector>

#include "../../entity/player/ServerPlayer.h"
#include "../Level.h"
#include "FractureTraits.h"

AfterimageFracture::AfterimageFracture() : FractureTrait("afterimage", FractureTraits::eGearType_Armor, 1)
{}

void AfterimageFracture::OnHurt(ServerPlayer *player, int pieces)
{
    if (player->GetHealth() <= 0 || player->GetLevel()->GetRandom().NextFloat() >= 0.04f * pieces)
    {
        return;
    }

    Level *level             = player->GetLevel();
    const glm::dvec3 &origin = player->GetPosition();
    std::vector<Entity *> entities;
    level->GetPushableEntities(player->GetAABB().Inflated(4.0), player, &entities);
    for (size_t i = 0; i < entities.size(); i++)
    {
        glm::dvec3 away = entities[i]->GetPosition() - origin;
        away.y          = 0.0;
        if (glm::length(away) < 1.0e-4)
        {
            continue;
        }

        glm::dvec3 push      = glm::normalize(away) * 1.2;
        LivingEntity *living = dynamic_cast<LivingEntity *>(entities[i]);
        if (living)
        {
            living->Knockback(origin, glm::dvec3(push.x, 0.2, push.z));
        }
        else
        {
            entities[i]->SetVelocity(glm::dvec3(push.x, 0.3, push.z));
        }

        level->BroadcastEntityEvent(entities[i], Entity::kAfterimageEvent);
    }

    glm::dvec3 center = origin + glm::dvec3(0.0, 1.0, 0.0);
    SendEffect(level, WorldEffects::eWorldEffectType_FracturePulse, center, 4.0f);
    SendParticles(level, eParticleType_Rift, center, glm::dvec3(1.5, 0.6, 1.5), 0.1f, 40);
    PlaySound(level, origin, 1.0f);
}
