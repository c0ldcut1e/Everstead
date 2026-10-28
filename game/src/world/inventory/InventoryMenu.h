#pragma once

#include <cstdint>
#include <vector>

#include "../../entity/Inventory.h"
#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

enum eCLICK_TYPE : uint8_t
{
    eClickType_Pickup    = 0,
    eClickType_QuickMove = 1,
    eClickType_Swap      = 2,
    eClickType_PickupAll = 3,
    eClickType_Drag      = 4,
    eClickType_Close     = 5,
    eClickType_Clone     = 6,
    eClickType_PickBlock = 7,
    eClickType_Throw     = 8,
    eClickType_Sort      = 9,
    eClickType_Count
};

class InventoryMenu
{
public:
    static const uint8_t s_playerMenuId         = 0;
    static const uint8_t s_chestMenuId          = 1;
    static const uint8_t s_creativeMenuId       = 2;
    static const uint8_t s_craftingMenuId       = 3;
    static const uint8_t s_recipeViewMenuId     = 4;
    static const uint8_t s_furnaceMenuId        = 5;
    static const uint8_t s_resonantSocketMenuId = 6;
    static const uint8_t s_resonatorMenuId      = 7;
    static const uint8_t s_hopperMenuId         = 8;
    static const uint8_t s_dispenserMenuId      = 9;
    static const uint8_t s_dropperMenuId        = 10;
    static const uint8_t s_anvilMenuId          = 11;
    static const int s_deleteSlot               = 999;
    static const int s_creativeSlotBase         = 1000;
    static const int s_creativeTabSlots         = 1000;
    static const int s_leftButton               = 0;
    static const int s_rightButton              = 1;
    static const int s_middleButton             = 2;
    static const int s_outsideSlot              = -999;
    static const int s_sortMode                 = 0;
    static const int s_sortRowsMode             = 1;
    static const int s_sortColumnsMode          = 2;

    InventoryMenu(Inventory *inventory, AbstractContainer *container, uint8_t menuId);

    int GetSlotCount() const;
    int GetPlayerSlotBase() const;
    bool IsStorage() const;
    ItemInstance *GetSlot(int index);
    const ItemInstance *GetDisplayItem(int index);

    static int GetCreativeSlot(int tab, int index);

    void Click(int slot, int button, eCLICK_TYPE type);
    void Drag(const std::vector<int> &slots, int button);
    int ComputeDrag(const std::vector<int> &slots, int button, std::vector<std::pair<int, ItemInstance>> *placed, ItemInstance *remaining);
    void Close();
    void TakeThrown(std::vector<ItemInstance> *thrown);

private:
    bool IsValidSlot(int slot) const;
    bool IsContainerSlot(int slot) const;
    int GetArmorIndex(int slot) const;
    bool FitsSlot(int slot, const ItemInstance &stack) const;
    int GetSlotLimit(int slot, const ItemInstance &stack) const;
    void ClickCreative(int slot, int button, eCLICK_TYPE type);
    void ClickResult(int button, eCLICK_TYPE type);
    void ConsumeInputs();
    void UpdateResult();
    void Pickup(int slot, int button);
    void QuickMove(int slot);
    void Swap(int slot, int hotbarSlot);
    void PickupAll(int slot);
    void Clone(int slot);
    void Throw(int slot, int button);
    void Sort(int firstSlot, int mode);
    void ThrowCarried(int button);
    void MoveTo(ItemInstance &stack, const std::vector<int> &targets);
    bool CanFit(const ItemInstance &stack, const std::vector<int> &targets);
    bool CanPlace(int slot, const ItemInstance &stack);

    Inventory *m_inventory;
    AbstractContainer *m_container;
    int m_containerSize;
    int m_containerBase;
    int m_columns;
    bool m_creative;
    bool m_crafting;
    bool m_furnace;
    bool m_resonantSocket;
    bool m_resonator;
    bool m_anvil;
    std::vector<ItemInstance> m_thrown;
};
