#pragma once

#include <vector>

#include "Ingredient.h"
#include "Recipe.h"

class ShapelessRecipe : public Recipe
{
public:
    ShapelessRecipe(const std::vector<Ingredient> &ingredients, const ItemInstance &result);

    bool Matches(const CraftingContainer &grid) const override;
    void FillGrid(CraftingContainer *grid, int cycle) const override;

private:
    std::vector<Ingredient> m_ingredients;
};
