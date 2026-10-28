#include "ResonanceFracture.h"

#include <algorithm>
#include <string>
#include <vector>

#include "../../entity/player/ServerPlayer.h"
#include "../Level.h"
#include "FractureTraits.h"

ResonanceFracture::ResonanceFracture() : FractureTrait("resonance", FractureTraits::eGearType_Weapon, 5)
{}

FractureTrait::State *ResonanceFracture::CreateState()
{
    return new ResonanceState();
}

bool ResonanceFracture::OnAttack(ServerPlayer *player, const Hit &hit)
{
    ResonanceState *state = (ResonanceState *) player->GetFractureState(this);
    state->hits           = !hit.missed && hit.target->GetId() == state->target ? state->hits + 1 : 1;
    state->target         = hit.target->GetId();
    if (state->hits < 4)
    {
        return false;
    }

    state->hits  = 0;
    Level *level = player->GetLevel();
    std::vector<Entity *> entities;
    level->GetPushableEntities(hit.target->GetAABB().Inflated(3.0), player, &entities);
    for (size_t i = 0; i < entities.size(); i++)
    {
        LivingEntity *living = dynamic_cast<LivingEntity *>(entities[i]);
        if (living && living != hit.target && living->Hurt(eDamageSource_Player, std::max(2, hit.damage / 2), player))
        {
            living->Knockback(hit.center, glm::dvec3());
        }
    }

    SendEffect(level, WorldEffects::eWorldEffectType_FracturePulse, hit.center, 3.0f);
    SendParticles(level, eParticleType_RiftSpark, hit.center, glm::dvec3(1.0), 0.3f, 30);
    level->PlaySound("fracture.resonance" + std::to_string(level->GetRandom().NextInt(1, 3)), hit.center, 1.0f,
                     0.9f + level->GetRandom().NextFloat() * 0.2f);
    return false;
}
