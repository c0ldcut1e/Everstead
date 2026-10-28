#include "ShapedRecipe.h"

ShapedRecipe::ShapedRecipe(int width, int height, const std::vector<Ingredient> &ingredients, const ItemInstance &result) : Recipe(result)
{
    m_width       = width;
    m_height      = height;
    m_ingredients = ingredients;
}

bool ShapedRecipe::Matches(const CraftingContainer &grid) const
{
    for (int x = 0; x <= grid.GetColumns() - m_width; x++)
    {
        for (int y = 0; y <= grid.GetRows() - m_height; y++)
        {
            if (MatchesAt(grid, x, y, false) || MatchesAt(grid, x, y, true))
            {
                return true;
            }
        }
    }

    return false;
}

void ShapedRecipe::FillGrid(CraftingContainer *grid, int cycle) const
{
    for (int y = 0; y < m_height; y++)
    {
        for (int x = 0; x < m_width; x++)
        {
            grid->SetItem(CraftingContainer::s_gridSlot + x + y * grid->GetColumns(),
                          m_ingredients[(size_t) (x + y * m_width)].GetDisplayItem(cycle));
        }
    }
}

bool ShapedRecipe::MatchesAt(const CraftingContainer &grid, int offsetX, int offsetY, bool mirrored) const
{
    Ingredient empty;
    for (int x = 0; x < grid.GetColumns(); x++)
    {
        for (int y = 0; y < grid.GetRows(); y++)
        {
            int patternX                 = x - offsetX;
            int patternY                 = y - offsetY;
            const Ingredient *ingredient = &empty;
            if (patternX >= 0 && patternY >= 0 && patternX < m_width && patternY < m_height)
            {
                ingredient = &m_ingredients[(size_t) ((mirrored ? m_width - patternX - 1 : patternX) + patternY * m_width)];
            }

            if (!ingredient->Test(*grid.GetGridItem(x, y)))
            {
                return false;
            }
        }
    }

    return true;
}
