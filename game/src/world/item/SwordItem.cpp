#include "SwordItem.h"

#include "../block/Block.h"

eUSE_ANIM SwordItem::GetUseAnimation() const
{
    return eUseAnim_Block;
}

int SwordItem::GetUseDuration() const
{
    return 72000;
}

float SwordItem::GetDestroySpeed(Block *block) const
{
    if (block == Block::ByName("bamboo") || block == Block::ByName("bamboo_sapling"))
    {
        return 30.0f;
    }

    return 1.5f;
}

int SwordItem::GetMineDamage(Block *block) const
{
    (void) block;
    return 2;
}

int SwordItem::GetHitDamage() const
{
    return 1;
}
