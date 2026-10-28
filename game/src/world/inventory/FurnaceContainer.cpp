#include "FurnaceContainer.h"

#include <memory>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../crafting/FurnaceRecipes.h"
#include "../item/Item.h"
#include "../save/WorldSave.h"

FurnaceContainer::FurnaceContainer()
{
    m_litTime     = 0;
    m_litDuration = 0;
    m_cookTime    = 0;
}

int FurnaceContainer::GetColumns() const
{
    return s_slotCount;
}

int FurnaceContainer::GetRows() const
{
    return 1;
}

int FurnaceContainer::GetSize() const
{
    return s_slotCount;
}

ItemInstance *FurnaceContainer::GetItem(int index)
{
    if (index < 0 || index >= s_slotCount)
    {
        return nullptr;
    }

    return &m_items[index];
}

const ItemInstance *FurnaceContainer::GetItem(int index) const
{
    if (index < 0 || index >= s_slotCount)
    {
        return nullptr;
    }

    return &m_items[index];
}

void FurnaceContainer::SetItem(int index, const ItemInstance &item)
{
    if (index < 0 || index >= s_slotCount)
    {
        return;
    }

    m_items[index] = item;
}

void FurnaceContainer::ClearItem(int index)
{
    SetItem(index, ItemInstance());
}

bool FurnaceContainer::IsEmpty() const
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

bool FurnaceContainer::Tick()
{
    bool wasLit  = IsLit();
    bool changed = false;
    if (m_litTime > 0)
    {
        m_litTime--;
    }

    if (m_litTime == 0 && CanBurn())
    {
        ItemInstance &fuel = m_items[s_fuelSlot];
        m_litDuration      = FurnaceRecipes::GetInstance()->GetBurnDuration(fuel);
        m_litTime          = m_litDuration;
        if (m_litTime > 0)
        {
            changed = true;
            fuel.SetCount(fuel.GetCount() - 1);
            if (fuel.GetCount() <= 0)
            {
                fuel = fuel.GetItem() == Item::ByName("lava_bucket") ? ItemInstance(Item::ByName("bucket")) : ItemInstance();
            }
        }
    }

    if (IsLit() && CanBurn())
    {
        m_cookTime++;
        if (m_cookTime == s_burnInterval)
        {
            m_cookTime = 0;
            Burn();
            changed = true;
        }
    }
    else
    {
        m_cookTime = 0;
    }

    return changed || wasLit != IsLit();
}

bool FurnaceContainer::IsLit() const
{
    return m_litTime > 0;
}

int FurnaceContainer::GetLitTime() const
{
    return m_litTime;
}

void FurnaceContainer::SetLitTime(int litTime)
{
    m_litTime = litTime;
}

int FurnaceContainer::GetLitDuration() const
{
    return m_litDuration;
}

void FurnaceContainer::SetLitDuration(int litDuration)
{
    m_litDuration = litDuration;
}

int FurnaceContainer::GetCookTime() const
{
    return m_cookTime;
}

void FurnaceContainer::SetCookTime(int cookTime)
{
    m_cookTime = cookTime;
}

bool FurnaceContainer::CanBurn() const
{
    ItemInstance result = FurnaceRecipes::GetInstance()->GetResult(m_items[s_inputSlot]);
    if (result.IsEmpty())
    {
        return false;
    }

    const ItemInstance &output = m_items[s_resultSlot];
    return output.IsEmpty() || (output.IsSameItem(result) && output.GetCount() < output.GetMaxStackSize());
}

void FurnaceContainer::Burn()
{
    ItemInstance &output = m_items[s_resultSlot];
    if (output.IsEmpty())
    {
        output = FurnaceRecipes::GetInstance()->GetResult(m_items[s_inputSlot]);
    }
    else
    {
        output.SetCount(output.GetCount() + 1);
    }

    ItemInstance &input = m_items[s_inputSlot];
    input.SetCount(input.GetCount() - 1);
    if (input.GetCount() <= 0)
    {
        input = ItemInstance();
    }
}

void FurnaceContainer::Save(CompoundTag *tag) const
{
    tag->PutShort("BurnTime", (int16_t) m_litTime);
    tag->PutShort("CookTime", (int16_t) m_cookTime);
    tag->PutShort("BurnDuration", (int16_t) m_litDuration);

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

void FurnaceContainer::Load(const CompoundTag *tag)
{
    m_litTime     = tag->GetShort("BurnTime");
    m_cookTime    = tag->GetShort("CookTime");
    m_litDuration = tag->GetShort("BurnDuration");

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
