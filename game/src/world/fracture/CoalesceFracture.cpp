#include "CoalesceFracture.h"

#include <algorithm>

#include "../item/Item.h"
#include "../item/ItemInstance.h"
#include "FractureTraits.h"

// The resonator never rolls this, it only comes from fractured books
CoalesceFracture::CoalesceFracture() : FractureTrait("coalesce", FractureTraits::eGearType_All, 0)
{}

int CoalesceFracture::OnPickupExperience(ServerPlayer *player, ItemInstance *item, int amount)
{
    (void) player;
    int repair = std::min(item->GetAuxValue(), amount * 2);
    item->SetAuxValue(item->GetAuxValue() - repair);
    return amount - (repair + 1) / 2;
}
