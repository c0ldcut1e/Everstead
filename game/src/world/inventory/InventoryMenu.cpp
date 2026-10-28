#include "InventoryMenu.h"

#include <algorithm>

#include "../crafting/FurnaceRecipes.h"
#include "../crafting/Recipes.h"
#include "../item/CreativeTabs.h"
#include "../item/Item.h"
#include "AnvilContainer.h"
#include "CraftingContainer.h"
#include "FurnaceContainer.h"
#include "ResonatorContainer.h"

InventoryMenu::InventoryMenu(Inventory *inventory, AbstractContainer *container, uint8_t menuId)
{
    m_inventory      = inventory;
    m_container      = container;
    m_containerSize  = container ? container->GetSize() : 0;
    m_containerBase  = menuId == s_playerMenuId || menuId == s_creativeMenuId ? Inventory::s_slotCount : 0;
    m_creative       = menuId == s_creativeMenuId;
    m_furnace        = menuId == s_furnaceMenuId;
    m_resonantSocket = menuId == s_resonantSocketMenuId;
    m_resonator      = menuId == s_resonatorMenuId;
    m_anvil          = menuId == s_anvilMenuId;
    m_crafting       = container && menuId != s_chestMenuId && menuId != s_hopperMenuId && menuId != s_dispenserMenuId && menuId != s_dropperMenuId &&
                       !m_furnace && !m_resonantSocket && !m_resonator && !m_anvil;
    m_columns        = Inventory::s_columns;
    if (menuId == s_hopperMenuId)
    {
        m_columns = m_containerSize;
    }
    else if (menuId == s_dispenserMenuId || menuId == s_dropperMenuId)
    {
        m_columns = 3;
    }
}

int InventoryMenu::GetSlotCount() const
{
    return m_containerSize + Inventory::s_slotCount;
}

const ItemInstance *InventoryMenu::GetDisplayItem(int index)
{
    if (index >= s_creativeSlotBase)
    {
        int creativeIndex = index - s_creativeSlotBase;
        return CreativeTabs::GetItem(creativeIndex / s_creativeTabSlots, creativeIndex % s_creativeTabSlots);
    }

    return GetSlot(index);
}

int InventoryMenu::GetCreativeSlot(int tab, int index)
{
    return s_creativeSlotBase + tab * s_creativeTabSlots + index;
}

int InventoryMenu::GetPlayerSlotBase() const
{
    return m_containerBase == 0 ? m_containerSize : 0;
}

bool InventoryMenu::IsStorage() const
{
    return m_container && m_containerBase == 0 && !m_crafting && !m_furnace && !m_resonantSocket && !m_resonator && !m_anvil;
}

bool InventoryMenu::IsContainerSlot(int slot) const
{
    return slot >= m_containerBase && slot < m_containerBase + m_containerSize;
}

ItemInstance *InventoryMenu::GetSlot(int index)
{
    if (!IsValidSlot(index))
    {
        return nullptr;
    }

    if (IsContainerSlot(index))
    {
        return m_container->GetItem(index - m_containerBase);
    }

    return m_inventory->GetSlot(index - GetPlayerSlotBase());
}

void InventoryMenu::Click(int slot, int button, eCLICK_TYPE type)
{
    if (type == eClickType_Close)
    {
        Close();
        return;
    }

    if (type == eClickType_Throw && slot == s_outsideSlot)
    {
        ThrowCarried(button);
        return;
    }

    if (type == eClickType_Sort)
    {
        Sort(slot, button);
        return;
    }

    if (m_creative && slot >= s_creativeSlotBase)
    {
        ClickCreative(slot, button, type);
        return;
    }

    if (slot == s_deleteSlot)
    {
        if (type == eClickType_QuickMove)
        {
            for (int i = 0; i < Inventory::s_slotCount; i++)
            {
                *m_inventory->GetSlot(i) = ItemInstance();
            }
        }

        m_inventory->GetCarried() = ItemInstance();
        return;
    }

    if (!IsValidSlot(slot))
    {
        return;
    }

    if ((m_crafting && slot == m_containerBase + CraftingContainer::s_resultSlot) || (m_anvil && slot == m_containerBase + AnvilContainer::s_resultSlot))
    {
        ClickResult(button, type);
        return;
    }

    switch (type)
    {
        case eClickType_Pickup:
            if (button == s_leftButton || button == s_rightButton)
            {
                Pickup(slot, button);
            }

            break;
        case eClickType_QuickMove:
            if (!m_creative)
            {
                QuickMove(slot);
            }
            else if (button == s_leftButton)
            {
                *GetSlot(slot) = ItemInstance();
            }

            break;
        case eClickType_Swap:
            if (button >= 0 && button < Inventory::s_hotbarSize)
            {
                Swap(slot, button);
            }

            break;
        case eClickType_PickupAll:
            PickupAll(slot);
            break;
        case eClickType_Clone:
            Clone(slot);
            break;
        case eClickType_Throw:
            Throw(slot, button);
            break;
        default:
            break;
    }

    UpdateResult();
}

void InventoryMenu::ClickResult(int button, eCLICK_TYPE type)
{
    ItemInstance &carried = m_inventory->GetCarried();
    int resultSlot        = m_anvil ? AnvilContainer::s_resultSlot : CraftingContainer::s_resultSlot;
    ItemInstance result   = *m_container->GetItem(resultSlot);
    if (result.IsEmpty() || (m_anvil && !((AnvilContainer *) m_container)->MayPickup()))
    {
        return;
    }

    switch (type)
    {
        case eClickType_Pickup:
            if (carried.IsEmpty())
            {
                carried = result;
            }
            else if (carried.IsSameItem(result) && carried.GetCount() + result.GetCount() <= carried.GetMaxStackSize())
            {
                carried.SetCount(carried.GetCount() + result.GetCount());
            }
            else
            {
                return;
            }

            ConsumeInputs();
            break;
        case eClickType_QuickMove: {
            int playerBase = GetPlayerSlotBase();
            std::vector<int> targets;
            for (int i = Inventory::s_hotbarSize; i < Inventory::s_totalSize; i++)
            {
                targets.push_back(playerBase + i);
            }

            for (int i = 0; i < Inventory::s_hotbarSize; i++)
            {
                targets.push_back(playerBase + i);
            }

            while (!result.IsEmpty() && CanFit(result, targets))
            {
                MoveTo(result, targets);
                ConsumeInputs();
                result = *m_container->GetItem(resultSlot);
            }

            break;
        }
        case eClickType_Swap:
            if (button >= 0 && button < Inventory::s_hotbarSize && carried.IsEmpty() && GetSlot(GetPlayerSlotBase() + button)->IsEmpty())
            {
                *GetSlot(GetPlayerSlotBase() + button) = result;
                ConsumeInputs();
            }

            break;
        case eClickType_Throw:
            if (!carried.IsEmpty())
            {
                return;
            }

            do
            {
                m_thrown.push_back(result);
                ConsumeInputs();
                result = *m_container->GetItem(resultSlot);
            } while (button == s_rightButton && !result.IsEmpty());

            break;
        default:
            break;
    }
}

void InventoryMenu::ConsumeInputs()
{
    if (m_anvil)
    {
        ((AnvilContainer *) m_container)->OnTake();
        return;
    }

    for (int i = CraftingContainer::s_gridSlot; i < m_containerSize; i++)
    {
        ItemInstance *item = m_container->GetItem(i);
        if (item->IsEmpty())
        {
            continue;
        }

        item->SetCount(item->GetCount() - 1);
        if (item->GetCount() <= 0)
        {
            *item = ItemInstance();
        }
    }

    UpdateResult();
}

void InventoryMenu::UpdateResult()
{
    if (m_crafting)
    {
        m_container->SetItem(CraftingContainer::s_resultSlot, Recipes::GetInstance()->GetResult(*(CraftingContainer *) m_container));
    }
    else if (m_anvil)
    {
        ((AnvilContainer *) m_container)->UpdateResult();
    }
}

int InventoryMenu::ComputeDrag(const std::vector<int> &slots, int button, std::vector<std::pair<int, ItemInstance>> *placed, ItemInstance *remaining)
{
    if (button != s_leftButton && button != s_rightButton)
    {
        return 0;
    }

    *remaining = m_inventory->GetCarried();
    std::vector<int> targets;
    for (size_t i = 0; i < slots.size(); i++)
    {
        if (!IsValidSlot(slots[i]) || std::find(targets.begin(), targets.end(), slots[i]) != targets.end())
        {
            return 0;
        }

        if (CanPlace(slots[i], *remaining))
        {
            targets.push_back(slots[i]);
        }
    }

    if (remaining->IsEmpty() || targets.empty())
    {
        return 0;
    }

    int perSlot = button == s_leftButton ? std::max(1, remaining->GetCount() / (int) targets.size()) : 1;
    for (size_t i = 0; i < targets.size() && !remaining->IsEmpty(); i++)
    {
        ItemInstance target = *GetSlot(targets[i]);
        int existing        = target.IsEmpty() ? 0 : target.GetCount();
        int moved           = std::min({perSlot, remaining->GetCount(), GetSlotLimit(targets[i], *remaining) - existing});
        if (moved <= 0)
        {
            continue;
        }

        if (target.IsEmpty())
        {
            target = *remaining;
            target.SetCount(moved);
        }
        else
        {
            target.SetCount(existing + moved);
        }

        placed->push_back({targets[i], target});
        remaining->SetCount(remaining->GetCount() - moved);
    }

    if (remaining->GetCount() <= 0)
    {
        *remaining = ItemInstance();
    }

    return (int) targets.size();
}

void InventoryMenu::Drag(const std::vector<int> &slots, int button)
{
    std::vector<std::pair<int, ItemInstance>> placed;
    ItemInstance remaining;
    int targetCount = ComputeDrag(slots, button, &placed, &remaining);
    if (targetCount == 0)
    {
        return;
    }

    if (targetCount == 1)
    {
        for (size_t i = 0; i < slots.size(); i++)
        {
            if (CanPlace(slots[i], m_inventory->GetCarried()))
            {
                Pickup(slots[i], button);
                return;
            }
        }
    }

    for (size_t i = 0; i < placed.size(); i++)
    {
        *GetSlot(placed[i].first) = placed[i].second;
    }

    m_inventory->GetCarried() = remaining;
    UpdateResult();
}

void InventoryMenu::Close()
{
    ItemInstance &carried = m_inventory->GetCarried();
    if (!carried.IsEmpty())
    {
        m_inventory->AddItem(carried);
    }

    // Anything that no longer fits is thrown out instead of being lost
    if (!carried.IsEmpty())
    {
        m_thrown.push_back(carried);
        carried = ItemInstance();
    }

    if (m_anvil)
    {
        ((AnvilContainer *) m_container)->SetItemName("");
    }
    else if (!m_crafting)
    {
        return;
    }

    int resultSlot = m_anvil ? AnvilContainer::s_resultSlot : CraftingContainer::s_resultSlot;
    m_container->ClearItem(resultSlot);
    for (int i = 0; i < m_containerSize; i++)
    {
        ItemInstance item = *m_container->GetItem(i);
        m_container->ClearItem(i);
        if (item.IsEmpty())
        {
            continue;
        }

        m_inventory->AddItem(item);
        if (!item.IsEmpty())
        {
            m_thrown.push_back(item);
        }
    }
}

void InventoryMenu::TakeThrown(std::vector<ItemInstance> *thrown)
{
    thrown->swap(m_thrown);
    m_thrown.clear();
}

// Left button throws a single item and right button throws the whole stack
void InventoryMenu::Throw(int slot, int button)
{
    ItemInstance *source = GetSlot(slot);
    if (!source || source->IsEmpty() || !m_inventory->GetCarried().IsEmpty())
    {
        return;
    }

    ItemInstance thrown = *source;
    int count           = button == s_rightButton ? source->GetCount() : 1;
    thrown.SetCount(count);
    source->SetCount(source->GetCount() - count);
    if (source->GetCount() <= 0)
    {
        *source = ItemInstance();
    }

    m_thrown.push_back(thrown);
}

void InventoryMenu::Sort(int firstSlot, int mode)
{
    int count   = Inventory::s_inventorySize;
    int columns = Inventory::s_columns;
    if (IsStorage() && firstSlot == 0)
    {
        count   = m_containerSize;
        columns = m_columns;
    }
    else if (firstSlot != GetPlayerSlotBase() + Inventory::s_hotbarSize)
    {
        return;
    }

    std::vector<ItemInstance> stacks;
    for (int i = 0; i < count; i++)
    {
        ItemInstance stack      = *GetSlot(firstSlot + i);
        *GetSlot(firstSlot + i) = ItemInstance();
        for (size_t j = 0; j < stacks.size() && !stack.IsEmpty(); j++)
        {
            int moved = std::min(stack.GetCount(), stacks[j].GetMaxStackSize() - stacks[j].GetCount());
            if (stacks[j].IsSameItem(stack) && moved > 0)
            {
                stacks[j].SetCount(stacks[j].GetCount() + moved);
                stack.SetCount(stack.GetCount() - moved);
            }
        }

        if (!stack.IsEmpty())
        {
            stacks.push_back(stack);
        }
    }

    const std::vector<ItemInstance> &order = CreativeTabs::GetItems(CreativeTabs::s_allItemsTab);
    std::vector<std::pair<size_t, ItemInstance>> ranked;
    for (size_t i = 0; i < stacks.size(); i++)
    {
        size_t rank = order.size();
        for (size_t j = 0; j < order.size(); j++)
        {
            if (order[j].GetItem() == stacks[i].GetItem() && (rank == order.size() || order[j].GetAuxValue() == stacks[i].GetAuxValue()))
            {
                rank = j;
                if (order[j].GetAuxValue() == stacks[i].GetAuxValue())
                {
                    break;
                }
            }
        }

        ranked.push_back({rank, stacks[i]});
    }

    std::stable_sort(ranked.begin(), ranked.end(),
                     [](const std::pair<size_t, ItemInstance> &a, const std::pair<size_t, ItemInstance> &b)
                     {
                         if (a.first != b.first)
                         {
                             return a.first < b.first;
                         }

                         if (a.second.GetItem()->GetId() != b.second.GetItem()->GetId())
                         {
                             return a.second.GetItem()->GetId() < b.second.GetItem()->GetId();
                         }

                         if (a.second.GetAuxValue() != b.second.GetAuxValue())
                         {
                             return a.second.GetAuxValue() < b.second.GetAuxValue();
                         }

                         return a.second.GetCount() > b.second.GetCount();
                     });

    for (size_t i = 0; i < ranked.size(); i++)
    {
        stacks[i] = ranked[i].second;
    }

    if (mode == s_sortMode)
    {
        for (size_t i = 0; i < stacks.size(); i++)
        {
            *GetSlot(firstSlot + (int) i) = stacks[i];
        }

        return;
    }

    std::vector<size_t> groups;
    for (size_t i = 0; i < stacks.size(); i++)
    {
        if (i == 0 || !stacks[i].IsSameItem(stacks[i - 1]))
        {
            groups.push_back(i);
        }
    }

    if (groups.empty())
    {
        return;
    }

    int distinctItems = (int) groups.size();
    groups.push_back(stacks.size());

    bool horizontal = mode == s_sortRowsMode;
    int rows        = count / columns;
    int lineLength  = horizontal ? columns : rows;
    std::vector<int> itemOrder;
    for (int i = 0; i < distinctItems; i++)
    {
        if ((int) (groups[i + 1] - groups[i]) > lineLength)
        {
            itemOrder.push_back(i);
        }
    }

    for (int i = 0; i < distinctItems; i++)
    {
        if ((int) (groups[i + 1] - groups[i]) <= lineLength)
        {
            itemOrder.push_back(i);
        }
    }

    int spaceWidth      = horizontal ? columns / ((distinctItems + rows - 1) / rows) : 1;
    int spaceHeight     = horizontal ? 1 : rows / ((distinctItems + columns - 1) / columns);
    int maxRow          = rows - 1;
    int maxColumn       = columns - 1;
    int row             = 0;
    int column          = 0;
    int availableSlots  = count;
    int remainingStacks = (int) stacks.size();
    std::vector<bool> filled(count, false);
    std::vector<bool> placed(stacks.size(), false);
    for (size_t i = 0; i < itemOrder.size(); i++)
    {
        size_t next = groups[itemOrder[i]];
        size_t end  = groups[itemOrder[i] + 1];
        int amount  = (int) (end - next);
        int width   = spaceWidth;
        int height  = spaceHeight;
        while (amount > width * height)
        {
            if (horizontal && column + width < maxColumn)
            {
                width = maxColumn - column + 1;
            }
            else if (horizontal && row + height < maxRow)
            {
                height++;
            }
            else if (!horizontal && row + height < maxRow)
            {
                height = maxRow - row + 1;
            }
            else if (!horizontal && column + width < maxColumn)
            {
                width++;
            }
            else
            {
                break;
            }
        }

        if (horizontal && column + width == maxColumn)
        {
            width++;
        }
        else if (!horizontal && row + height == maxRow)
        {
            height++;
        }

        int outer = horizontal ? height : width;
        int inner = horizontal ? width : height;
        for (int a = 0; a < outer && next < end; a++)
        {
            for (int b = 0; b < inner && next < end; b++)
            {
                int slot = horizontal ? (row + a) * columns + column + b : (row + b) * columns + column + a;
                if (!filled[slot])
                {
                    *GetSlot(firstSlot + slot) = stacks[next];
                    filled[slot]               = true;
                    placed[next]               = true;
                    next++;
                }
            }
        }

        availableSlots -= width * height;
        remainingStacks -= amount;
        if (availableSlots < remainingStacks)
        {
            break;
        }

        if (horizontal && column + width + spaceWidth <= maxColumn + 1)
        {
            column += width;
        }
        else if (horizontal)
        {
            column = 0;
            row += height;
        }
        else if (row + height + spaceHeight <= maxRow + 1)
        {
            row += height;
        }
        else
        {
            row = 0;
            column += width;
        }

        if (row > maxRow || column > maxColumn)
        {
            break;
        }
    }

    // Stacks that got no space of their own fill the bottom rows or the right columns
    size_t next = 0;
    for (int a = 0; a < (horizontal ? rows : columns); a++)
    {
        for (int b = 0; b < (horizontal ? columns : rows); b++)
        {
            int slot = horizontal ? (maxRow - a) * columns + b : b * columns + maxColumn - a;
            while (next < stacks.size() && placed[next])
            {
                next++;
            }

            if (!filled[slot] && next < stacks.size())
            {
                *GetSlot(firstSlot + slot) = stacks[next];
                placed[next]               = true;
            }
        }
    }
}

void InventoryMenu::ThrowCarried(int button)
{
    ItemInstance &carried = m_inventory->GetCarried();
    if (carried.IsEmpty())
    {
        return;
    }

    ItemInstance thrown = carried;
    int count           = button == s_rightButton ? 1 : carried.GetCount();
    thrown.SetCount(count);
    carried.SetCount(carried.GetCount() - count);
    if (carried.GetCount() <= 0)
    {
        carried = ItemInstance();
    }

    m_thrown.push_back(thrown);
}

bool InventoryMenu::IsValidSlot(int slot) const
{
    return slot >= 0 && slot < GetSlotCount();
}

int InventoryMenu::GetArmorIndex(int slot) const
{
    int index = slot - GetPlayerSlotBase() - Inventory::s_totalSize;
    return index >= 0 && index < Inventory::s_armorSize ? index : -1;
}

bool InventoryMenu::FitsSlot(int slot, const ItemInstance &stack) const
{
    if (m_crafting && slot == m_containerBase + CraftingContainer::s_resultSlot && !stack.IsEmpty())
    {
        return false;
    }

    if (m_furnace && slot == m_containerBase + FurnaceContainer::s_resultSlot && !stack.IsEmpty())
    {
        return false;
    }

    if (m_anvil && slot == m_containerBase + AnvilContainer::s_resultSlot && !stack.IsEmpty())
    {
        return false;
    }

    if (m_resonantSocket && IsContainerSlot(slot) && !stack.IsEmpty() && stack.GetItem() != Item::ByName("resonant_shard"))
    {
        return false;
    }

    if (m_resonator && IsContainerSlot(slot) && !stack.IsEmpty())
    {
        int index = slot - m_containerBase;
        if (index == ResonatorContainer::s_resultSlot || (index == ResonatorContainer::s_materialSlot && !ResonatorContainer::IsMaterial(stack)) ||
            (index == ResonatorContainer::s_gearSlot && !ResonatorContainer::IsGear(stack)))
        {
            return false;
        }
    }

    int armorIndex = GetArmorIndex(slot);
    return armorIndex < 0 || stack.IsEmpty() || stack.GetItem()->GetArmorSlot() == armorIndex;
}

int InventoryMenu::GetSlotLimit(int slot, const ItemInstance &stack) const
{
    return m_resonantSocket && IsContainerSlot(slot) ? 1 : stack.GetMaxStackSize();
}

void InventoryMenu::ClickCreative(int slot, int button, eCLICK_TYPE type)
{
    const ItemInstance *source = GetDisplayItem(slot);
    ItemInstance &carried      = m_inventory->GetCarried();
    if (type == eClickType_Swap)
    {
        if (source && button >= 0 && button < Inventory::s_hotbarSize)
        {
            ItemInstance *target = GetSlot(GetPlayerSlotBase() + button);
            *target              = *source;
            target->SetCount(target->GetMaxStackSize());
        }

        return;
    }

    if (type == eClickType_PickBlock)
    {
        if (source && button >= 0 && button < Inventory::s_hotbarSize)
        {
            ItemInstance picked = *source;
            picked.SetCount(1);
            m_inventory->SetPickedItem(button, picked);
        }

        return;
    }

    if (type == eClickType_Throw)
    {
        if (source && carried.IsEmpty())
        {
            ItemInstance thrown = *source;
            thrown.SetCount(button == s_rightButton ? thrown.GetMaxStackSize() : 1);
            m_thrown.push_back(thrown);
        }

        return;
    }

    if (type == eClickType_Clone)
    {
        if (source && carried.IsEmpty())
        {
            carried = *source;
            carried.SetCount(carried.GetMaxStackSize());
        }

        return;
    }

    if (type != eClickType_Pickup && type != eClickType_QuickMove)
    {
        return;
    }

    if (source && !carried.IsEmpty() && carried.IsSameItem(*source))
    {
        if (button == s_leftButton)
        {
            carried.SetCount(type == eClickType_QuickMove ? carried.GetMaxStackSize() : std::min(carried.GetCount() + 1, carried.GetMaxStackSize()));
        }
        else if (carried.GetCount() <= 1)
        {
            carried = ItemInstance();
        }
        else
        {
            carried.SetCount(carried.GetCount() - 1);
        }
    }
    else if (source && carried.IsEmpty())
    {
        carried = *source;
        carried.SetCount(type == eClickType_QuickMove ? carried.GetMaxStackSize() : 1);
    }
    else
    {
        carried = ItemInstance();
    }
}

void InventoryMenu::Pickup(int slot, int button)
{
    ItemInstance *target  = GetSlot(slot);
    ItemInstance &carried = m_inventory->GetCarried();

    if (carried.IsEmpty())
    {
        if (target->IsEmpty())
        {
            return;
        }

        int taken = button == s_leftButton ? target->GetCount() : (target->GetCount() + 1) / 2;
        carried   = *target;
        carried.SetCount(taken);
        target->SetCount(target->GetCount() - taken);
        if (target->GetCount() <= 0)
        {
            *target = ItemInstance();
        }

        return;
    }

    if ((m_furnace && slot == m_containerBase + FurnaceContainer::s_resultSlot) ||
        (m_resonator && slot == m_containerBase + ResonatorContainer::s_resultSlot))
    {
        int taken = target->IsSameItem(carried) ? std::min(target->GetCount(), carried.GetMaxStackSize() - carried.GetCount()) : 0;
        carried.SetCount(carried.GetCount() + taken);
        target->SetCount(target->GetCount() - taken);
        if (target->GetCount() <= 0)
        {
            *target = ItemInstance();
        }

        return;
    }

    if (!FitsSlot(slot, carried))
    {
        return;
    }

    if (target->IsEmpty())
    {
        int placed = button == s_leftButton ? carried.GetCount() : 1;
        placed     = std::min(placed, GetSlotLimit(slot, carried));
        *target    = carried;
        target->SetCount(placed);
        carried.SetCount(carried.GetCount() - placed);
    }
    else if (target->IsSameItem(carried))
    {
        int wanted = button == s_leftButton ? carried.GetCount() : 1;
        int placed = std::min(wanted, GetSlotLimit(slot, *target) - target->GetCount());
        if (placed > 0)
        {
            target->SetCount(target->GetCount() + placed);
            carried.SetCount(carried.GetCount() - placed);
        }
    }
    else if (carried.GetCount() <= GetSlotLimit(slot, carried))
    {
        std::swap(*target, carried);
    }

    if (carried.GetCount() <= 0)
    {
        carried = ItemInstance();
    }
}

void InventoryMenu::QuickMove(int slot)
{
    ItemInstance *source = GetSlot(slot);
    if (source->IsEmpty())
    {
        return;
    }

    int playerBase = GetPlayerSlotBase();
    int armorSlot  = source->GetItem()->GetArmorSlot();
    std::vector<int> targets;
    if (m_crafting && IsContainerSlot(slot))
    {
        for (int i = Inventory::s_hotbarSize; i < Inventory::s_totalSize; i++)
        {
            targets.push_back(playerBase + i);
        }

        for (int i = 0; i < Inventory::s_hotbarSize; i++)
        {
            targets.push_back(playerBase + i);
        }
    }
    else if ((m_furnace || m_resonantSocket || m_resonator || m_anvil) && IsContainerSlot(slot))
    {
        for (int i = Inventory::s_hotbarSize - 1; i >= 0; i--)
        {
            targets.push_back(playerBase + i);
        }

        for (int i = Inventory::s_totalSize - 1; i >= Inventory::s_hotbarSize; i--)
        {
            targets.push_back(playerBase + i);
        }
    }
    else if (m_anvil)
    {
        targets.push_back(m_containerBase + AnvilContainer::s_inputSlot);
        targets.push_back(m_containerBase + AnvilContainer::s_additionSlot);
    }
    else if (m_resonantSocket && source->GetItem() == Item::ByName("resonant_shard"))
    {
        targets.push_back(m_containerBase);
    }
    else if (m_resonator && ResonatorContainer::IsMaterial(*source))
    {
        targets.push_back(m_containerBase + ResonatorContainer::s_materialSlot);
    }
    else if (m_resonator && ResonatorContainer::IsGear(*source))
    {
        targets.push_back(m_containerBase + ResonatorContainer::s_gearSlot);
    }
    else if (m_furnace && !FurnaceRecipes::GetInstance()->GetResult(*source).IsEmpty())
    {
        targets.push_back(m_containerBase + FurnaceContainer::s_inputSlot);
    }
    else if (m_furnace && FurnaceRecipes::GetInstance()->GetBurnDuration(*source) > 0)
    {
        targets.push_back(m_containerBase + FurnaceContainer::s_fuelSlot);
    }
    else if (armorSlot >= 0 && m_containerBase == Inventory::s_slotCount && GetArmorIndex(slot) < 0 &&
             GetSlot(playerBase + Inventory::s_totalSize + armorSlot)->IsEmpty())
    {
        targets.push_back(playerBase + Inventory::s_totalSize + armorSlot);
    }
    else if (GetArmorIndex(slot) >= 0)
    {
        for (int i = Inventory::s_hotbarSize; i < Inventory::s_totalSize; i++)
        {
            targets.push_back(playerBase + i);
        }

        for (int i = 0; i < Inventory::s_hotbarSize; i++)
        {
            targets.push_back(playerBase + i);
        }
    }
    else if (m_container && !m_crafting && !m_furnace && !m_resonantSocket && !m_resonator)
    {
        if (IsContainerSlot(slot))
        {
            for (int i = Inventory::s_hotbarSize - 1; i >= 0; i--)
            {
                targets.push_back(playerBase + i);
            }

            for (int i = Inventory::s_totalSize - 1; i >= Inventory::s_hotbarSize; i--)
            {
                targets.push_back(playerBase + i);
            }
        }
        else
        {
            for (int i = 0; i < m_containerSize; i++)
            {
                targets.push_back(m_containerBase + i);
            }
        }
    }
    else if (slot - playerBase < Inventory::s_hotbarSize)
    {
        for (int i = Inventory::s_hotbarSize; i < Inventory::s_totalSize; i++)
        {
            targets.push_back(playerBase + i);
        }
    }
    else
    {
        for (int i = 0; i < Inventory::s_hotbarSize; i++)
        {
            targets.push_back(playerBase + i);
        }
    }

    ItemInstance stack = *source;
    MoveTo(stack, targets);
    *source = stack;
}

void InventoryMenu::Swap(int slot, int hotbarSlot)
{
    if (!m_inventory->GetCarried().IsEmpty())
    {
        return;
    }

    int hotbarIndex = GetPlayerSlotBase() + hotbarSlot;
    if (hotbarIndex == slot || !FitsSlot(slot, *GetSlot(hotbarIndex)) || GetSlot(hotbarIndex)->GetCount() > GetSlotLimit(slot, *GetSlot(hotbarIndex)))
    {
        return;
    }

    std::swap(*GetSlot(slot), *GetSlot(hotbarIndex));
}

void InventoryMenu::PickupAll(int slot)
{
    ItemInstance &carried = m_inventory->GetCarried();
    if (carried.IsEmpty())
    {
        return;
    }

    for (int pass = 0; pass < 2; pass++)
    {
        for (int index = 0; index < GetSlotCount() && carried.GetCount() < carried.GetMaxStackSize(); index++)
        {
            ItemInstance *source = GetSlot(index);
            if ((index == slot && pass == 0) || (m_crafting && index == m_containerBase + CraftingContainer::s_resultSlot) ||
                (m_anvil && index == m_containerBase + AnvilContainer::s_resultSlot))
            {
                continue;
            }

            if (source->IsEmpty() || !source->IsSameItem(carried))
            {
                continue;
            }

            bool full = source->GetCount() >= source->GetMaxStackSize();
            if (pass == 0 && full)
            {
                continue;
            }

            int taken = std::min(source->GetCount(), carried.GetMaxStackSize() - carried.GetCount());
            carried.SetCount(carried.GetCount() + taken);
            source->SetCount(source->GetCount() - taken);
            if (source->GetCount() <= 0)
            {
                *source = ItemInstance();
            }
        }
    }
}

void InventoryMenu::Clone(int slot)
{
    ItemInstance &carried = m_inventory->GetCarried();
    ItemInstance *source  = GetSlot(slot);
    if (!carried.IsEmpty() || source->IsEmpty())
    {
        return;
    }

    carried = *source;
    carried.SetCount(carried.GetMaxStackSize());
}

void InventoryMenu::MoveTo(ItemInstance &stack, const std::vector<int> &targets)
{
    for (int pass = 0; pass < 2 && !stack.IsEmpty(); pass++)
    {
        for (size_t i = 0; i < targets.size() && !stack.IsEmpty(); i++)
        {
            ItemInstance *target = GetSlot(targets[i]);
            if (!FitsSlot(targets[i], stack))
            {
                continue;
            }

            if (pass == 0)
            {
                if (target->IsEmpty() || !target->IsSameItem(stack))
                {
                    continue;
                }

                int moved = std::min(stack.GetCount(), GetSlotLimit(targets[i], stack) - target->GetCount());
                if (moved > 0)
                {
                    target->SetCount(target->GetCount() + moved);
                    stack.SetCount(stack.GetCount() - moved);
                }
            }
            else if (target->IsEmpty())
            {
                int moved = std::min(stack.GetCount(), GetSlotLimit(targets[i], stack));
                *target   = stack;
                target->SetCount(moved);
                stack.SetCount(stack.GetCount() - moved);
            }
        }
    }

    if (stack.GetCount() <= 0)
    {
        stack = ItemInstance();
    }
}

bool InventoryMenu::CanFit(const ItemInstance &stack, const std::vector<int> &targets)
{
    int space = 0;
    for (size_t i = 0; i < targets.size(); i++)
    {
        ItemInstance *target = GetSlot(targets[i]);
        if (target->IsEmpty())
        {
            space += stack.GetMaxStackSize();
        }
        else if (target->IsSameItem(stack))
        {
            space += target->GetMaxStackSize() - target->GetCount();
        }
    }

    return space >= stack.GetCount();
}

bool InventoryMenu::CanPlace(int slot, const ItemInstance &stack)
{
    ItemInstance *target = GetSlot(slot);
    if (stack.IsEmpty() || !FitsSlot(slot, stack))
    {
        return false;
    }

    return target->IsEmpty() || (target->IsSameItem(stack) && target->GetCount() < GetSlotLimit(slot, stack));
}
