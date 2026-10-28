#include "PhaseFracture.h"

#include <memory>

#include "../../entity/player/ServerPlayer.h"
#include "../../network/listener/PlayerConnection.h"
#include "../../network/packet/s2c/WorldEffectS2CPacket.h"
#include "FractureTraits.h"

PhaseFracture::PhaseFracture() : FractureTrait("phase", FractureTraits::eGearType_Armor, 2)
{}

bool PhaseFracture::PreventHurt(ServerPlayer *player, eDAMAGE_SOURCE source, int pieces)
{
    if (!LivingEntity::IsArmorDamage(source) || player->GetInvulnerableTime() > LivingEntity::s_invulnerableDuration / 2 ||
        player->GetLevel()->GetRandom().NextFloat() >= 0.025f * pieces)
    {
        return false;
    }

    player->SetInvulnerableTime(LivingEntity::s_invulnerableDuration);
    glm::dvec3 eye = player->GetPosition() + glm::dvec3(0.0, player->GetEyeHeight(), 0.0);
    player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new WorldEffectS2CPacket(WorldEffects::eWorldEffectType_PhaseGlitch, eye, 1.5f)));
    PlaySound(player->GetLevel(), player->GetPosition(), 0.8f);
    return true;
}
