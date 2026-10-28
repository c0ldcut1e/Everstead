#pragma once

#include <cstdint>
#include <vector>

#include "../../utils/Random.h"
#include "FractureTrait.h"

class Item;
class ItemInstance;

class FractureTraits
{
public:
    static constexpr int kMaxFractures = 3;

    enum eGEAR_TYPE : uint8_t
    {
        eGearType_None   = 0,
        eGearType_Weapon = 1 << 0,
        eGearType_Tool   = 1 << 1,
        eGearType_Armor  = 1 << 2,
        eGearType_Bow    = 1 << 3,
        eGearType_All    = eGearType_Weapon | eGearType_Tool | eGearType_Armor | eGearType_Bow
    };

    static const std::vector<FractureTrait *> &GetTraits();
    static FractureTrait *ByLocation(const ResourceLocation &id);
    static eGEAR_TYPE GetGearType(const Item *item);

    // Unknown ids in the list still count toward the limit so traits removed later keep their slot
    static int GetCount(const ItemInstance &item);
    static float GetGlint(const ItemInstance &item);
    static std::vector<FractureTrait *> Get(const ItemInstance &item);
    // Fractures only work on gear they fit, so a held fractured book does nothing
    static std::vector<FractureTrait *> GetActive(const ItemInstance &item);
    static bool CanApply(const ItemInstance &item, FractureTrait *trait);
    static void Apply(ItemInstance *item, FractureTrait *trait);
    static void Clear(ItemInstance *item);
    static FractureTrait *Roll(const ItemInstance &item, Random *random);
};
