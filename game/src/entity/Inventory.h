#pragma once

#include "../world/item/ItemInstance.h"

class Inventory
{
public:
    Inventory();

    int GetSelectedSlot() const;
    void SetSelectedSlot(int slot);
    void ScrollSelectedSlot(int delta);

    ItemInstance *GetItem(int slot);
    void SetItem(int slot, const ItemInstance &item);
    ItemInstance *GetSelectedItem();
    ItemInstance *GetInventoryItem(int slot);
    void SetInventoryItem(int slot, const ItemInstance &item);

    ItemInstance *GetArmor(int slot);
    const ItemInstance *GetArmor(int slot) const;

    ItemInstance *GetSlot(int index);
    void SetSlot(int index, const ItemInstance &item);

    ItemInstance &GetCarried();
    void SetCarried(const ItemInstance &item);

    void AddItem(ItemInstance &item);
    void SetPickedItem(int slot, const ItemInstance &item);
    ItemInstance *FindItem(Item *item);

    float GetPopTime(int slot) const;
    void SetPopTime(int slot, float time);

    static const int s_columns       = 9;
    static const int s_hotbarSize    = s_columns;
    static const int s_inventoryRows = 3;
    static const int s_inventorySize = s_columns * s_inventoryRows;
    static const int s_totalSize     = s_hotbarSize + s_inventorySize;
    static const int s_armorSize     = 4;
    static const int s_slotCount     = s_totalSize + s_armorSize;

private:
    int m_selectedSlot;
    ItemInstance m_hotbar[s_hotbarSize];
    ItemInstance m_inventory[s_inventorySize];
    ItemInstance m_armor[s_armorSize];
    ItemInstance m_carried;
    float m_popTime[s_hotbarSize];
};
