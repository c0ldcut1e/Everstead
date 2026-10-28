#pragma once

#include <vector>

#include "Ingredient.h"
#include "Recipe.h"

class ShapedRecipe : public Recipe
{
public:
    ShapedRecipe(int width, int height, const std::vector<Ingredient> &ingredients, const ItemInstance &result);

    bool Matches(const CraftingContainer &grid) const override;
    void FillGrid(CraftingContainer *grid, int cycle) const override;

private:
    bool MatchesAt(const CraftingContainer &grid, int offsetX, int offsetY, bool mirrored) const;

    int m_width;
    int m_height;
    std::vector<Ingredient> m_ingredients;
};
