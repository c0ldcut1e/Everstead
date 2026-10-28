#pragma once

#include "Item.h"

class SwordItem : public Item
{
public:
    eUSE_ANIM GetUseAnimation() const override;
    int GetUseDuration() const override;
    float GetDestroySpeed(Block *block) const override;
    int GetMineDamage(Block *block) const override;
    int GetHitDamage() const override;
};
