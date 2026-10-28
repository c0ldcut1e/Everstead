#include "UndertowFracture.h"

#include "../../entity/player/ServerPlayer.h"
#include "FractureTraits.h"

UndertowFracture::UndertowFracture() : FractureTrait("undertow", FractureTraits::eGearType_Weapon, 5)
{}

bool UndertowFracture::OnAttack(ServerPlayer *player, const Hit &hit)
{
    if (hit.target->GetHealth() <= 0 || hit.target->GetLevel()->GetRandom().NextFloat() >= 0.2f)
    {
        return false;
    }

    hit.target->Knockback(hit.target->GetPosition() * 2.0 - player->GetPosition(), glm::dvec3());
    SendParticles(player->GetLevel(), eParticleType_Rift, hit.center, glm::dvec3(0.4), 0.05f, 16);
    PlaySound(player->GetLevel(), hit.center, 1.0f);
    return true;
}
