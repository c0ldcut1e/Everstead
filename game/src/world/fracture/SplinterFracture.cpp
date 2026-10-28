#include "SplinterFracture.h"

#include <algorithm>

#include "../../entity/player/ServerPlayer.h"
#include "../Level.h"
#include "FractureTraits.h"

SplinterFracture::SplinterFracture() : FractureTrait("splinter", FractureTraits::eGearType_Weapon, 2)
{}

FractureTrait::State *SplinterFracture::CreateState()
{
    return new SplinterState();
}

void SplinterFracture::Tick(ServerPlayer *player, State *state)
{
    SplinterState *splinter = (SplinterState *) state;
    if (splinter->ticks <= 0 || --splinter->ticks > 0)
    {
        return;
    }

    Level *level         = player->GetLevel();
    LivingEntity *target = dynamic_cast<LivingEntity *>(level->GetEntityById(splinter->target));
    if (!target || target->GetHealth() <= 0)
    {
        return;
    }

    const AABB &box   = target->GetAABB();
    glm::dvec3 center = (box.GetMin() + box.GetMax()) * 0.5;
    target->SetInvulnerableTime(0);
    target->Hurt(eDamageSource_Player, splinter->damage, player);
    SendParticles(level, eParticleType_RiftSpark, center, glm::dvec3(0.3), 0.3f, 14);
    PlaySound(level, center, 1.0f);
}

bool SplinterFracture::OnAttack(ServerPlayer *player, const Hit &hit)
{
    if (hit.target->GetHealth() <= 0 || hit.target->GetLevel()->GetRandom().NextFloat() >= 0.15f)
    {
        return false;
    }

    SplinterState *state = (SplinterState *) player->GetFractureState(this);
    state->target        = hit.target->GetId();
    state->ticks         = 4;
    state->damage        = std::max(1, hit.damage / 2);
    SendParticles(player->GetLevel(), eParticleType_RiftSpark, hit.center, glm::dvec3(0.2), 0.1f, 6);
    return false;
}
