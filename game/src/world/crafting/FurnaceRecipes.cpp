#include "FurnaceRecipes.h"

#include "../block/SaplingBlock.h"
#include "../block/SignBlock.h"
#include "../block/SlabBlock.h"
#include "../inventory/FurnaceContainer.h"
#include "../item/Item.h"

FurnaceRecipes *FurnaceRecipes::GetInstance()
{
    static FurnaceRecipes instance;
    return &instance;
}

void FurnaceRecipes::Init()
{
    FurnaceRecipes *recipes = GetInstance();
    recipes->AddRecipe("iron_ore", "iron_ingot", 0.7f);
    recipes->AddRecipe("gold_ore", "gold_ingot", 1.0f);
    recipes->AddRecipe("diamond_ore", "diamond", 1.0f);
    recipes->AddRecipe("sand", "glass", 0.1f);
    recipes->AddRecipe("porkchop", "cooked_porkchop", 0.35f);
    recipes->AddRecipe("beef", "cooked_beef", 0.35f);
    recipes->AddRecipe("chicken", "cooked_chicken", 0.35f);
    recipes->AddRecipe("cobblestone", "stone", 0.1f);
    recipes->AddRecipe("stone", "smooth_stone", 0.1f);
    recipes->AddRecipe("cactus", "green_dye", 0.2f);
    recipes->AddRecipe("emerald_ore", "emerald", 1.0f);
    recipes->AddRecipe("potato", "baked_potato", 0.35f);
    recipes->AddRecipe("coal_ore", "coal", 0.1f);
    recipes->AddRecipe("lapis_ore", "lapis_lazuli", 0.2f);
    recipes->AddRecipe("red_sand", "glass", 0.1f);
    recipes->AddRecipe("echo_quartz", "echo_glass", 0.1f);
    recipes->AddRecipe("clay", "terracotta", 0.35f);
    recipes->AddRecipe("sandstone", "smooth_sandstone", 0.1f);
    recipes->AddRecipe("sandstone_iron_ore", "iron_ingot", 0.7f);
    recipes->AddRecipe("sandstone_gold_ore", "gold_ingot", 1.0f);
    recipes->AddRecipe("sandstone_diamond_ore", "diamond", 1.0f);
    recipes->AddRecipe("sandstone_emerald_ore", "emerald", 1.0f);
    recipes->AddRecipe("sandstone_coal_ore", "coal", 0.1f);
    recipes->AddRecipe("sandstone_lapis_ore", "lapis_lazuli", 0.2f);
    recipes->AddRecipe("ice_iron_ore", "iron_ingot", 0.7f);
    recipes->AddRecipe("ice_gold_ore", "gold_ingot", 1.0f);
    recipes->AddRecipe("ice_diamond_ore", "diamond", 1.0f);
    recipes->AddRecipe("ice_emerald_ore", "emerald", 1.0f);
    recipes->AddRecipe("ice_coal_ore", "coal", 0.1f);
    recipes->AddRecipe("ice_lapis_ore", "lapis_lazuli", 0.2f);
    recipes->AddRecipe("basalt_iron_ore", "iron_ingot", 0.7f);
    recipes->AddRecipe("basalt_gold_ore", "gold_ingot", 1.0f);
    recipes->AddRecipe("basalt_diamond_ore", "diamond", 1.0f);
    recipes->AddRecipe("basalt_emerald_ore", "emerald", 1.0f);
    recipes->AddRecipe("basalt_coal_ore", "coal", 0.1f);
    recipes->AddRecipe("basalt_lapis_ore", "lapis_lazuli", 0.2f);
    recipes->AddRecipe("basalt_quartz_ore", "quartz", 0.2f);
    recipes->AddRecipe("redstone_ore", "redstone", 0.7f);
    recipes->AddRecipe("sandstone_redstone_ore", "redstone", 0.7f);
    recipes->AddRecipe("ice_redstone_ore", "redstone", 0.7f);
    recipes->AddRecipe("basalt_redstone_ore", "redstone", 0.7f);
    recipes->AddRecipe("red_sandstone", "smooth_red_sandstone", 0.1f);
    recipes->AddRecipe("stone_bricks", "cracked_stone_bricks", 0.1f);

    const char *logs[] = {"oak_log", "spruce_log", "birch_log", "acacia_log", "jungle_log", "cherry_log", "dark_oak_log"};
    for (size_t i = 0; i < std::size(logs); i++)
    {
        recipes->AddRecipe(logs[i], "charcoal", 0.15f);
    }

    const char *ironItems[] = {"iron_pickaxe",         "iron_axe",           "iron_shovel",    "iron_sword", "iron_hoe",
                               "iron_helmet",          "iron_chestplate",    "iron_leggings",  "iron_boots", "chainmail_helmet",
                               "chainmail_chestplate", "chainmail_leggings", "chainmail_boots"};
    for (size_t i = 0; i < std::size(ironItems); i++)
    {
        recipes->AddRecipe(ironItems[i], "iron_nugget", 0.1f);
    }

    const char *goldItems[] = {"golden_pickaxe", "golden_axe",        "golden_shovel",   "golden_sword", "golden_hoe",
                               "golden_helmet",  "golden_chestplate", "golden_leggings", "golden_boots"};
    for (size_t i = 0; i < std::size(goldItems); i++)
    {
        recipes->AddRecipe(goldItems[i], "gold_nugget", 0.1f);
    }

    const char *woodenTools[] = {"wooden_sword", "wooden_shovel", "wooden_pickaxe", "wooden_axe", "wooden_hoe"};
    for (size_t i = 0; i < std::size(woodenTools); i++)
    {
        recipes->AddFuel(woodenTools[i], FurnaceContainer::s_burnInterval);
    }

    recipes->AddFuel("stick", FurnaceContainer::s_burnInterval / 2);
    recipes->AddFuel("coal", FurnaceContainer::s_burnInterval * 8);
    recipes->AddFuel("charcoal", FurnaceContainer::s_burnInterval * 8);
    recipes->AddFuel("coal_block", FurnaceContainer::s_burnInterval * 80);
    recipes->AddFuel("bow", FurnaceContainer::s_burnInterval);
    recipes->AddFuel("bowl", FurnaceContainer::s_burnInterval / 2);
    recipes->AddFuel("lava_bucket", FurnaceContainer::s_burnInterval * 100);
}

void FurnaceRecipes::AddRecipe(const std::string &input, const std::string &result, float experience)
{
    m_inputs.push_back(Item::ByName(input));
    m_results[Item::ByName(input)]     = Item::ByName(result);
    m_experience[Item::ByName(result)] = experience;
}

void FurnaceRecipes::AddFuel(const std::string &item, int duration)
{
    m_fuels.push_back(Item::ByName(item));
    m_burnDurations[Item::ByName(item)] = duration;
}

ItemInstance FurnaceRecipes::GetResult(const ItemInstance &input) const
{
    if (input.IsEmpty())
    {
        return ItemInstance();
    }

    auto result = m_results.find(input.GetItem());
    return result == m_results.end() ? ItemInstance() : ItemInstance(result->second);
}

std::vector<Item *> FurnaceRecipes::FindInputs(const Item *result) const
{
    std::vector<Item *> inputs;
    for (size_t i = 0; i < m_inputs.size(); i++)
    {
        if (m_results.at(m_inputs[i]) == result)
        {
            inputs.push_back(m_inputs[i]);
        }
    }

    return inputs;
}

const std::vector<Item *> &FurnaceRecipes::GetFuels() const
{
    return m_fuels;
}

int FurnaceRecipes::GetBurnDuration(const ItemInstance &fuel) const
{
    if (fuel.IsEmpty())
    {
        return 0;
    }

    Block *block = fuel.GetItem()->GetBlock();
    if (dynamic_cast<SignBlock *>(block))
    {
        return FurnaceContainer::s_burnInterval;
    }

    if (block && block->GetToolType() == Block::eToolType_Axe)
    {
        return dynamic_cast<SlabBlock *>(block) ? FurnaceContainer::s_burnInterval * 3 / 4 : FurnaceContainer::s_burnInterval * 3 / 2;
    }

    if (dynamic_cast<SaplingBlock *>(block))
    {
        return FurnaceContainer::s_burnInterval / 2;
    }

    auto duration = m_burnDurations.find(fuel.GetItem());
    return duration == m_burnDurations.end() ? 0 : duration->second;
}

float FurnaceRecipes::GetExperience(const Item *result) const
{
    auto experience = m_experience.find(result);
    return experience == m_experience.end() ? 0.0f : experience->second;
}
