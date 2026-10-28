#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../item/ItemInstance.h"

class Item;

class FurnaceRecipes
{
public:
    static FurnaceRecipes *GetInstance();
    static void Init();

    ItemInstance GetResult(const ItemInstance &input) const;
    std::vector<Item *> FindInputs(const Item *result) const;
    const std::vector<Item *> &GetFuels() const;
    int GetBurnDuration(const ItemInstance &fuel) const;
    float GetExperience(const Item *result) const;

private:
    void AddRecipe(const std::string &input, const std::string &result, float experience);
    void AddFuel(const std::string &item, int duration);

    std::vector<Item *> m_inputs;
    std::vector<Item *> m_fuels;
    std::unordered_map<const Item *, Item *> m_results;
    std::unordered_map<const Item *, int> m_burnDurations;
    std::unordered_map<const Item *, float> m_experience;
};
