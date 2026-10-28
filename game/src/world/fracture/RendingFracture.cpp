#include "RendingFracture.h"

#include <algorithm>

#include "FractureTraits.h"

RendingFracture::RendingFracture() : FractureTrait("rending", FractureTraits::eGearType_Weapon, 10)
{}

int RendingFracture::ModifyAttackDamage(ServerPlayer *player, int damage)
{
    (void) player;
    return damage + std::max(1, damage / 4);
}

int RendingFracture::GetAttackWear()
{
    return 1;
}
