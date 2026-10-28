#pragma once

#include <string>
#include <vector>

#include "ItemInstance.h"

class CreativeTabs
{
public:
    static const int s_tabCount     = 7;
    static const int s_inventoryTab = 6;
    static const int s_allItemsTab  = 5;

    static const std::vector<ItemInstance> &GetItems(int tab);
    static const ItemInstance *GetItem(int tab, int index);
    static const ItemInstance &GetIcon(int tab);
    static std::string GetNameKey(int tab);
};
