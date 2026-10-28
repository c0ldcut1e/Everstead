#include "ResonatorContainer.h"

#include <memory>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../fracture/FractureTraits.h"
#include "../item/Item.h"
#include "../save/WorldSave.h"

ResonatorContainer::ResonatorContainer()
{
    m_progress = 0;
}

int ResonatorContainer::GetColumns() const
{
    return s_slotCount;
}

int ResonatorContainer::GetRows() const
{
    return 1;
}

int ResonatorContainer::GetSize() const
{
    return s_slotCount;
}

ItemInstance *ResonatorContainer::GetItem(int index)
{
    if (index < 0 || index >= s_slotCount)
    {
        return nullptr;
    }

    return &m_items[index];
}

const ItemInstance *ResonatorContainer::GetItem(int index) const
{
    if (index < 0 || index >= s_slotCount)
    {
        return nullptr;
    }

    return &m_items[index];
}

void ResonatorContainer::SetItem(int index, const ItemInstance &item)
{
    if (index < 0 || index >= s_slotCount)
    {
        return;
    }

    m_items[index] = item;
}

void ResonatorContainer::ClearItem(int index)
{
    SetItem(index, ItemInstance());
}

bool ResonatorContainer::IsEmpty() const
{
    for (int i = 0; i < s_slotCount; i++)
    {
        if (!m_items[i].IsEmpty())
        {
            return false;
        }
    }

    return true;
}

Item *ResonatorContainer::GetUpgradeResult(const ItemInstance &gear)
{
    if (gear.IsEmpty())
    {
        return nullptr;
    }

    const std::string &name = gear.GetItem()->GetResourceLocation().GetPath();
    if (name.rfind("diamond_", 0) != 0)
    {
        return nullptr;
    }

    return Item::ByName("severite_" + name.substr(8));
}

bool ResonatorContainer::IsGear(const ItemInstance &item)
{
    return !item.IsEmpty() && (GetUpgradeResult(item) || FractureTraits::GetGearType(item.GetItem()) != FractureTraits::eGearType_None);
}

bool ResonatorContainer::IsMaterial(const ItemInstance &item)
{
    Item *type = item.GetItem();
    return !item.IsEmpty() && (type == Item::ByName("severite") || type == Item::ByName("rift_ember") || type == Item::ByName("echo_seed"));
}

bool ResonatorContainer::Tick(Random *random, eRESONATOR_MODE *completed)
{
    *completed           = eResonatorMode_None;
    eRESONATOR_MODE mode = GetMode();
    if (mode == eResonatorMode_None)
    {
        bool changed = m_progress != 0;
        m_progress   = 0;
        return changed;
    }

    m_progress++;
    if (m_progress < s_workDuration)
    {
        return true;
    }

    ItemInstance &gear     = m_items[s_gearSlot];
    ItemInstance &material = m_items[s_materialSlot];
    ItemInstance &result   = m_items[s_resultSlot];
    m_progress             = 0;
    *completed             = mode;
    if (mode == eResonatorMode_Upgrade)
    {
        result = ItemInstance(GetUpgradeResult(gear), 1, gear.GetAuxValue());
        if (gear.HasTag())
        {
            result.SetTag((CompoundTag *) gear.GetTag()->Clone().release());
        }
    }
    else
    {
        result = gear;
        if (mode == eResonatorMode_Fracture)
        {
            FractureTraits::Apply(&result, FractureTraits::Roll(gear, random));
        }
        else
        {
            FractureTraits::Clear(&result);
        }
    }

    gear = ItemInstance();
    material.SetCount(material.GetCount() - 1);
    if (material.GetCount() <= 0)
    {
        material = ItemInstance();
    }

    return true;
}

int ResonatorContainer::GetProgress() const
{
    return m_progress;
}

void ResonatorContainer::SetProgress(int progress)
{
    m_progress = progress;
}

ResonatorContainer::eRESONATOR_MODE ResonatorContainer::GetMode() const
{
    const ItemInstance &gear     = m_items[s_gearSlot];
    const ItemInstance &material = m_items[s_materialSlot];
    if (!m_items[s_resultSlot].IsEmpty() || gear.IsEmpty() || material.IsEmpty())
    {
        return eResonatorMode_None;
    }

    Item *type = material.GetItem();
    if (type == Item::ByName("severite") && GetUpgradeResult(gear))
    {
        return eResonatorMode_Upgrade;
    }

    if (type == Item::ByName("echo_seed") && FractureTraits::GetCount(gear) > 0)
    {
        return eResonatorMode_Stabilize;
    }

    const std::vector<FractureTrait *> &traits = FractureTraits::GetTraits();
    for (size_t i = 0; type == Item::ByName("rift_ember") && i < traits.size(); i++)
    {
        if (FractureTraits::CanApply(gear, traits[i]))
        {
            return eResonatorMode_Fracture;
        }
    }

    return eResonatorMode_None;
}

void ResonatorContainer::Save(CompoundTag *tag) const
{
    tag->PutShort("Progress", (int16_t) m_progress);

    std::unique_ptr<ListTag> items(new ListTag());
    for (int i = 0; i < s_slotCount; i++)
    {
        if (m_items[i].IsEmpty())
        {
            continue;
        }

        std::unique_ptr<CompoundTag> itemTag = WorldSave::CreateItemTag(m_items[i]);
        itemTag->PutByte("Slot", (int8_t) i);
        items->Add(std::move(itemTag));
    }

    tag->Put("Items", std::move(items));
}

void ResonatorContainer::Load(const CompoundTag *tag)
{
    m_progress = tag->GetShort("Progress");

    ListTag *items = tag->GetList("Items");
    for (size_t i = 0; items && i < items->Size(); i++)
    {
        const CompoundTag *itemTag = (const CompoundTag *) items->Get(i);
        int slot                   = itemTag->GetByte("Slot");
        if (slot >= 0 && slot < s_slotCount)
        {
            m_items[slot] = WorldSave::ReadItemTag(itemTag);
        }
    }
}
