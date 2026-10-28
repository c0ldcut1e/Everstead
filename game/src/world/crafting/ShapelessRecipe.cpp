#include "ShapelessRecipe.h"

#include <cstddef>

ShapelessRecipe::ShapelessRecipe(const std::vector<Ingredient> &ingredients, const ItemInstance &result) : Recipe(result)
{
    m_ingredients = ingredients;
}

bool ShapelessRecipe::Matches(const CraftingContainer &grid) const
{
    std::vector<const Ingredient *> remaining;
    for (size_t i = 0; i < m_ingredients.size(); i++)
    {
        remaining.push_back(&m_ingredients[i]);
    }

    for (int i = CraftingContainer::s_gridSlot; i < grid.GetSize(); i++)
    {
        const ItemInstance *item = grid.GetItem(i);
        if (item->IsEmpty())
        {
            continue;
        }

        bool found = false;
        for (size_t j = 0; j < remaining.size(); j++)
        {
            if (remaining[j]->Test(*item))
            {
                remaining.erase(remaining.begin() + (ptrdiff_t) j);
                found = true;
                break;
            }
        }

        if (!found)
        {
            return false;
        }
    }

    return remaining.empty();
}

void ShapelessRecipe::FillGrid(CraftingContainer *grid, int cycle) const
{
    for (size_t i = 0; i < m_ingredients.size(); i++)
    {
        grid->SetItem(CraftingContainer::s_gridSlot + (int) i, m_ingredients[i].GetDisplayItem(cycle));
    }
}
