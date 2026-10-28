#include "FractureTraits.h"

#include <algorithm>
#include <memory>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../tag/StringTag.h"
#include "../item/BowItem.h"
#include "../item/DiggerItem.h"
#include "../item/ItemInstance.h"
#include "../item/SwordItem.h"
#include "AfterimageFracture.h"
#include "CoalesceFracture.h"
#include "EchoingFracture.h"
#include "FaultLineFracture.h"
#include "OverflowFracture.h"
#include "PhaseFracture.h"
#include "RecallFracture.h"
#include "RefractionFracture.h"
#include "RendingFracture.h"
#include "ResonanceFracture.h"
#include "RicochetFracture.h"
#include "SeamFracture.h"
#include "ShatterpointFracture.h"
#include "SplinterFracture.h"
#include "TearFracture.h"
#include "TemperedFracture.h"
#include "UndertowFracture.h"

const std::vector<FractureTrait *> &FractureTraits::GetTraits()
{
    static const std::vector<FractureTrait *> traits = {
            new RendingFracture(),    new PhaseFracture(),     new EchoingFracture(),  new SplinterFracture(),     new AfterimageFracture(),
            new FaultLineFracture(),  new ResonanceFracture(), new UndertowFracture(), new ShatterpointFracture(), new SeamFracture(),
            new RefractionFracture(), new OverflowFracture(),  new TearFracture(),     new RicochetFracture(),     new RecallFracture(),
            new CoalesceFracture(),   new TemperedFracture()};
    return traits;
}

FractureTrait *FractureTraits::ByLocation(const ResourceLocation &id)
{
    const std::vector<FractureTrait *> &traits = GetTraits();
    for (size_t i = 0; i < traits.size(); i++)
    {
        if (traits[i]->GetId() == id)
        {
            return traits[i];
        }
    }

    return nullptr;
}

FractureTraits::eGEAR_TYPE FractureTraits::GetGearType(const Item *item)
{
    if (dynamic_cast<const SwordItem *>(item))
    {
        return eGearType_Weapon;
    }

    if (dynamic_cast<const BowItem *>(item))
    {
        return eGearType_Bow;
    }

    if (dynamic_cast<const DiggerItem *>(item))
    {
        return eGearType_Tool;
    }

    return item->GetArmorSlot() >= 0 ? eGearType_Armor : eGearType_None;
}

int FractureTraits::GetCount(const ItemInstance &item)
{
    CompoundTag *tag = item.GetTag();
    return tag && tag->Contains("Fractures", Tag::eTagType_List) ? (int) tag->GetList("Fractures")->Size() : 0;
}

float FractureTraits::GetGlint(const ItemInstance &item)
{
    return (float) std::min(GetCount(item), kMaxFractures) / kMaxFractures;
}

std::vector<FractureTrait *> FractureTraits::Get(const ItemInstance &item)
{
    std::vector<FractureTrait *> result;
    if (GetCount(item) == 0)
    {
        return result;
    }

    ListTag *fractures = item.GetTag()->GetList("Fractures");
    for (size_t i = 0; i < fractures->Size(); i++)
    {
        FractureTrait *trait =
                fractures->Get(i)->GetType() == Tag::eTagType_String ? ByLocation(((StringTag *) fractures->Get(i))->GetValue()) : nullptr;
        if (trait)
        {
            result.push_back(trait);
        }
    }

    return result;
}

std::vector<FractureTrait *> FractureTraits::GetActive(const ItemInstance &item)
{
    std::vector<FractureTrait *> result = Get(item);
    if (result.empty())
    {
        return result;
    }

    uint8_t gearType = GetGearType(item.GetItem());
    result.erase(std::remove_if(result.begin(), result.end(),
                                [gearType](FractureTrait *trait)
                                {
                                    return !(trait->GetGearTypes() & gearType);
                                }),
                 result.end());
    return result;
}

bool FractureTraits::CanApply(const ItemInstance &item, FractureTrait *trait)
{
    if (item.IsEmpty() || !(trait->GetGearTypes() & GetGearType(item.GetItem())) || GetCount(item) >= kMaxFractures)
    {
        return false;
    }

    std::vector<FractureTrait *> current = Get(item);
    for (size_t i = 0; i < current.size(); i++)
    {
        if (current[i] == trait)
        {
            return false;
        }
    }

    return true;
}

void FractureTraits::Apply(ItemInstance *item, FractureTrait *trait)
{
    if (!item->HasTag())
    {
        item->SetTag(new CompoundTag());
    }

    CompoundTag *tag = item->GetTag();
    if (!tag->Contains("Fractures", Tag::eTagType_List))
    {
        tag->Put("Fractures", std::unique_ptr<ListTag>(new ListTag()));
    }

    tag->GetList("Fractures")->Add(std::unique_ptr<StringTag>(new StringTag(trait->GetId().ToString())));
}

void FractureTraits::Clear(ItemInstance *item)
{
    if (!item->HasTag())
    {
        return;
    }

    item->GetTag()->Remove("Fractures");
    if (item->GetTag()->GetTags().empty())
    {
        item->SetTag(nullptr);
    }
}

FractureTrait *FractureTraits::Roll(const ItemInstance &item, Random *random)
{
    const std::vector<FractureTrait *> &traits = GetTraits();
    int totalWeight                            = 0;
    for (size_t i = 0; i < traits.size(); i++)
    {
        if (CanApply(item, traits[i]))
        {
            totalWeight += traits[i]->GetWeight();
        }
    }

    if (totalWeight == 0)
    {
        return nullptr;
    }

    int pick = random->NextInt(0, totalWeight - 1);
    for (size_t i = 0; i < traits.size(); i++)
    {
        if (!CanApply(item, traits[i]))
        {
            continue;
        }

        pick -= traits[i]->GetWeight();
        if (pick < 0)
        {
            return traits[i];
        }
    }

    return nullptr;
}
