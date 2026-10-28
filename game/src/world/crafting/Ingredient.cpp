#include "Ingredient.h"

#include <algorithm>

Ingredient::Ingredient()
{}

Ingredient::Ingredient(const std::vector<std::string> &names)
{
    for (size_t i = 0; i < names.size(); i++)
    {
        m_items.push_back(Item::ByName(names[i]));
    }
}

bool Ingredient::IsEmpty() const
{
    return m_items.empty();
}

bool Ingredient::Test(const ItemInstance &item) const
{
    if (item.IsEmpty())
    {
        return m_items.empty();
    }

    return std::find(m_items.begin(), m_items.end(), item.GetItem()) != m_items.end();
}

ItemInstance Ingredient::GetDisplayItem(int cycle) const
{
    return m_items.empty() ? ItemInstance() : ItemInstance(m_items[(size_t) cycle % m_items.size()], 1);
}
