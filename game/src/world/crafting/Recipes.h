#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Ingredient.h"
#include "Recipe.h"

class Recipes
{
public:
    static Recipes *GetInstance();
    static void Init();

    ItemInstance GetResult(const CraftingContainer &grid) const;
    std::vector<const Recipe *> FindRecipes(const Item *item) const;

private:
    void AddShaped(const std::string &result, int count, const std::vector<std::string> &pattern,
                   const std::vector<std::pair<char, Ingredient>> &keys);
    void AddShapeless(const std::string &result, int count, const std::vector<Ingredient> &ingredients);
    void AddWoodRecipes();
    void AddStoneRecipes();
    void AddToolRecipes();
    void AddDyeRecipes();
    void AddMiscRecipes();
    void AddDecorationRecipes();

    std::vector<std::unique_ptr<Recipe>> m_recipes;
};
