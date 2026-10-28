#include "ShatterpointFracture.h"

#include "../../entity/player/ServerPlayer.h"
#include "FractureTraits.h"

ShatterpointFracture::ShatterpointFracture() : FractureTrait("shatterpoint", FractureTraits::eGearType_Weapon, 2)
{}

FractureTrait::State *ShatterpointFracture::CreateState()
{
    return new ShatterpointState();
}

int ShatterpointFracture::ModifyAttackDamage(ServerPlayer *player, int damage)
{
    ShatterpointState *state = (ShatterpointState *) player->GetFractureState(this);
    state->shattered         = player->GetLevel()->GetRandom().NextFloat() < 0.08f;
    return state->shattered ? damage * 2 : damage;
}

bool ShatterpointFracture::OnAttack(ServerPlayer *player, const Hit &hit)
{
    ShatterpointState *state = (ShatterpointState *) player->GetFractureState(this);
    if (!state->shattered)
    {
        return false;
    }

    state->shattered = false;
    Level *level     = player->GetLevel();
    SendEffect(level, WorldEffects::eWorldEffectType_FractureCrack, hit.center, 0.8f);
    SendParticles(level, eParticleType_Crit, hit.center, glm::dvec3(0.3), 0.4f, 20);
    PlaySound(level, hit.center, 1.0f);
    return false;
}
