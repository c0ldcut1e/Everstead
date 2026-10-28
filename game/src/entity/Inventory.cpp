#include "Inventory.h"

#include <algorithm>

Inventory::Inventory()
{
    m_selectedSlot = 0;
    for (int i = 0; i < s_hotbarSize; i++)
    {
        m_popTime[i] = 0.0f;
    }
}

int Inventory::GetSelectedSlot() const
{
    return m_selectedSlot;
}

void Inventory::SetSelectedSlot(int slot)
{
    if (slot < 0 || slot >= s_hotbarSize)
    {
        return;
    }

    m_selectedSlot = slot;
}

void Inventory::ScrollSelectedSlot(int delta)
{
    int slot = (m_selectedSlot + delta) % s_hotbarSize;
    if (slot < 0)
    {
        slot += s_hotbarSize;
    }

    m_selectedSlot = slot;
}

ItemInstance *Inventory::GetItem(int slot)
{
    if (slot < 0 || slot >= s_hotbarSize)
    {
        return nullptr;
    }

    return &m_hotbar[slot];
}

void Inventory::SetItem(int slot, const ItemInstance &item)
{
    if (slot < 0 || slot >= s_hotbarSize)
    {
        return;
    }

    m_hotbar[slot] = item;
}

ItemInstance *Inventory::GetSelectedItem()
{
    return GetItem(m_selectedSlot);
}

ItemInstance *Inventory::GetInventoryItem(int slot)
{
    if (slot < 0 || slot >= s_inventorySize)
    {
        return nullptr;
    }

    return &m_inventory[slot];
}

void Inventory::SetInventoryItem(int slot, const ItemInstance &item)
{
    if (slot < 0 || slot >= s_inventorySize)
    {
        return;
    }

    m_inventory[slot] = item;
}

ItemInstance *Inventory::GetArmor(int slot)
{
    return &m_armor[slot];
}

const ItemInstance *Inventory::GetArmor(int slot) const
{
    return &m_armor[slot];
}

ItemInstance *Inventory::GetSlot(int index)
{
    if (index < s_hotbarSize)
    {
        return GetItem(index);
    }

    if (index >= s_totalSize)
    {
        return &m_armor[index - s_totalSize];
    }

    return GetInventoryItem(index - s_hotbarSize);
}

void Inventory::SetSlot(int index, const ItemInstance &item)
{
    if (index < s_hotbarSize)
    {
        SetItem(index, item);
        return;
    }

    if (index >= s_totalSize)
    {
        m_armor[index - s_totalSize] = item;
        return;
    }

    SetInventoryItem(index - s_hotbarSize, item);
}

ItemInstance &Inventory::GetCarried()
{
    return m_carried;
}

void Inventory::SetCarried(const ItemInstance &item)
{
    m_carried = item;
}

ItemInstance *Inventory::FindItem(Item *item)
{
    for (int index = 0; index < s_totalSize; index++)
    {
        ItemInstance *slot = GetSlot(index);
        if (!slot->IsEmpty() && slot->GetItem() == item)
        {
            return slot;
        }
    }

    return nullptr;
}

void Inventory::AddItem(ItemInstance &item)
{
    for (int pass = 0; pass < 2 && !item.IsEmpty(); pass++)
    {
        for (int index = 0; index < s_totalSize && !item.IsEmpty(); index++)
        {
            ItemInstance *slot = GetSlot(index);
            if (pass == 0)
            {
                if (slot->IsEmpty() || !slot->IsSameItem(item))
                {
                    continue;
                }

                int moved = std::min(item.GetCount(), slot->GetMaxStackSize() - slot->GetCount());
                if (moved <= 0)
                {
                    continue;
                }

                slot->SetCount(slot->GetCount() + moved);
                item.SetCount(item.GetCount() - moved);
            }
            else if (slot->IsEmpty())
            {
                *slot = item;
                item  = ItemInstance();
            }
        }
    }
}

void Inventory::SetPickedItem(int slot, const ItemInstance &item)
{
    for (int index = 0; index < s_totalSize && !m_hotbar[slot].IsEmpty(); index++)
    {
        if (GetSlot(index)->IsEmpty())
        {
            *GetSlot(index) = m_hotbar[slot];
            break;
        }
    }

    m_hotbar[slot] = item;
}

float Inventory::GetPopTime(int slot) const
{
    return m_popTime[slot];
}

void Inventory::SetPopTime(int slot, float time)
{
    m_popTime[slot] = time;
}
