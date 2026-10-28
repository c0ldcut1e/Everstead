#pragma once

#include "Item.h"

class MilkBucketItem : public Item
{
public:
    eUSE_ANIM GetUseAnimation() const override;
    int GetUseDuration() const override;
};
