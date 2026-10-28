#pragma once

#include <string>
#include <vector>

#include "../item/Item.h"
#include "../item/ItemInstance.h"

class Ingredient
{
public:
    Ingredient();
    Ingredient(const std::vector<std::string> &names);

    bool IsEmpty() const;
    bool Test(const ItemInstance &item) const;
    ItemInstance GetDisplayItem(int cycle) const;

private:
    std::vector<Item *> m_items;
};
