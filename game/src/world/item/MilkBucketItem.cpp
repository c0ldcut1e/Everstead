#include "MilkBucketItem.h"

eUSE_ANIM MilkBucketItem::GetUseAnimation() const
{
    return eUseAnim_Drink;
}

int MilkBucketItem::GetUseDuration() const
{
    return 32;
}
