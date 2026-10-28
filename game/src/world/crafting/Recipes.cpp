#include "Recipes.h"

#include <iterator>
#include <map>

#include "../item/Item.h"
#include "ShapedRecipe.h"
#include "ShapelessRecipe.h"

Recipes *Recipes::GetInstance()
{
    static Recipes instance;
    return &instance;
}

void Recipes::Init()
{
    Recipes *recipes = GetInstance();
    recipes->AddWoodRecipes();
    recipes->AddStoneRecipes();
    recipes->AddToolRecipes();
    recipes->AddDyeRecipes();
    recipes->AddMiscRecipes();
    recipes->AddDecorationRecipes();
}

ItemInstance Recipes::GetResult(const CraftingContainer &grid) const
{
    for (size_t i = 0; i < m_recipes.size(); i++)
    {
        if (!m_recipes[i]->Matches(grid))
        {
            continue;
        }

        return m_recipes[i]->GetResult();
    }

    return ItemInstance();
}

std::vector<const Recipe *> Recipes::FindRecipes(const Item *item) const
{
    std::vector<const Recipe *> recipes;
    for (size_t i = 0; i < m_recipes.size(); i++)
    {
        if (m_recipes[i]->GetResult().GetItem() == item)
        {
            recipes.push_back(m_recipes[i].get());
        }
    }

    return recipes;
}

void Recipes::AddShaped(const std::string &result, int count, const std::vector<std::string> &pattern,
                        const std::vector<std::pair<char, Ingredient>> &keys)
{
    std::map<char, Ingredient> keyMap(keys.begin(), keys.end());
    std::vector<Ingredient> ingredients;
    for (size_t y = 0; y < pattern.size(); y++)
    {
        for (size_t x = 0; x < pattern[y].size(); x++)
        {
            ingredients.push_back(pattern[y][x] == ' ' ? Ingredient() : keyMap.at(pattern[y][x]));
        }
    }

    m_recipes.push_back(
            std::make_unique<ShapedRecipe>((int) pattern[0].size(), (int) pattern.size(), ingredients, ItemInstance(Item::ByName(result), count)));
}

void Recipes::AddShapeless(const std::string &result, int count, const std::vector<Ingredient> &ingredients)
{
    m_recipes.push_back(std::make_unique<ShapelessRecipe>(ingredients, ItemInstance(Item::ByName(result), count)));
}

void Recipes::AddWoodRecipes()
{
    constexpr const char *kWoods[] = {"oak", "spruce", "birch", "acacia", "jungle", "cherry", "dark_oak", "bamboo"};
    std::vector<std::string> allPlanks;
    for (const char *wood : kWoods)
    {
        std::string name = wood;
        Ingredient planks({name + "_planks"});
        Ingredient stick({"stick"});
        allPlanks.push_back(name + "_planks");

        if (name == "bamboo")
        {
            AddShapeless(name + "_planks", 2, {Ingredient({"bamboo_block"})});
        }
        else
        {
            AddShapeless(name + "_planks", 4, {Ingredient({name + "_log"})});
        }

        AddShaped(name + "_door", 3, {"##", "##", "##"}, {{'#', planks}});
        AddShaped(name + "_trapdoor", 2, {"###", "###"}, {{'#', planks}});
        AddShaped(name + "_fence", 3, {"W#W", "W#W"}, {{'W', planks}, {'#', stick}});
        AddShaped(name + "_fence_gate", 1, {"#W#", "#W#"}, {{'W', planks}, {'#', stick}});
        AddShapeless(name + "_button", 1, {planks});
        AddShaped(name + "_pressure_plate", 1, {"##"}, {{'#', planks}});
        AddShaped(name + "_slab", 6, {"###"}, {{'#', planks}});
        AddShaped(name + "_stairs", 4, {"#  ", "## ", "###"}, {{'#', planks}});
        AddShaped(name + "_sign", 3, {"###", "###", " X "}, {{'#', planks}, {'X', stick}});
        if (name != "bamboo")
        {
            AddShaped(name + "_boat", 1, {"# #", "###"}, {{'#', planks}});
        }
    }

    Ingredient planks(allPlanks);
    Ingredient stick({"stick"});
    AddShaped("stick", 4, {"#", "#"}, {{'#', planks}});
    AddShaped("crafting_table", 1, {"##", "##"}, {{'#', planks}});
    AddShaped("chest", 1, {"###", "# #", "###"}, {{'#', planks}});
    AddShaped("piston", 1, {"TTT", "#X#", "#R#"},
              {{'T', planks}, {'#', Ingredient({"cobblestone"})}, {'X', Ingredient({"iron_ingot"})}, {'R', Ingredient({"redstone"})}});
    AddShaped("note_block", 1, {"###", "#X#", "###"}, {{'#', planks}, {'X', Ingredient({"redstone"})}});
    AddShaped("target", 1, {" R ", "RHR", " R "}, {{'R', Ingredient({"redstone"})}, {'H', Ingredient({"hay_block"})}});
    AddShaped("bookshelf", 1, {"###", "BBB", "###"}, {{'#', planks}, {'B', Ingredient({"book"})}});
    AddShaped("ladder", 3, {"# #", "###", "# #"}, {{'#', stick}});
    AddShaped("stick", 1, {"#", "#"}, {{'#', Ingredient({"bamboo"})}});
    AddShaped("bamboo_block", 1, {"###", "###", "###"}, {{'#', Ingredient({"bamboo"})}});
    AddShaped("bamboo_mosaic", 1, {"#", "#"}, {{'#', Ingredient({"bamboo_slab"})}});
    AddShaped("bamboo_mosaic_slab", 6, {"###"}, {{'#', Ingredient({"bamboo_mosaic"})}});
    AddShaped("bamboo_mosaic_stairs", 4, {"#  ", "## ", "###"}, {{'#', Ingredient({"bamboo_mosaic"})}});
    AddShapeless("pink_dye", 1, {Ingredient({"pink_petals"})});
    AddShapeless("black_dye", 1, {Ingredient({"ink_sac"})});
    AddShaped("scaffolding", 6, {"I~I", "I I", "I I"}, {{'I', Ingredient({"bamboo"})}, {'~', Ingredient({"string"})}});
    AddShaped("bowl", 4, {"# #", " # "}, {{'#', planks}});
    AddShaped("torch", 4, {"X", "#"}, {{'X', Ingredient({"coal", "charcoal"})}, {'#', stick}});
}

void Recipes::AddStoneRecipes()
{
    Ingredient stone({"stone"});
    Ingredient cobblestone({"cobblestone"});
    Ingredient stoneBricks({"stone_bricks"});
    Ingredient mossyStoneBricks({"mossy_stone_bricks"});
    Ingredient vine({"vine"});

    AddShaped("furnace", 1, {"###", "# #", "###"}, {{'#', cobblestone}});
    AddShaped("stone_bricks", 4, {"##", "##"}, {{'#', stone}});
    AddShaped("stone_slab", 6, {"###"}, {{'#', stone}});
    AddShaped("stone_stairs", 4, {"#  ", "## ", "###"}, {{'#', stone}});
    AddShaped("stone_brick_slab", 6, {"###"}, {{'#', stoneBricks}});
    AddShaped("stone_brick_stairs", 4, {"#  ", "## ", "###"}, {{'#', stoneBricks}});
    AddShaped("mossy_stone_brick_slab", 6, {"###"}, {{'#', mossyStoneBricks}});
    AddShaped("mossy_stone_brick_stairs", 4, {"#  ", "## ", "###"}, {{'#', mossyStoneBricks}});
    AddShaped("chiseled_stone_bricks", 1, {"#", "#"}, {{'#', Ingredient({"stone_brick_slab"})}});
    AddShapeless("mossy_cobblestone", 1, {cobblestone, vine});
    AddShapeless("mossy_stone_bricks", 1, {stoneBricks, vine});
    AddShapeless("stone_button", 1, {stone});
    AddShaped("lever", 1, {"X", "#"}, {{'X', Ingredient({"stick"})}, {'#', cobblestone}});
    AddShaped("rail", 16, {"X X", "X#X", "X X"}, {{'X', Ingredient({"iron_ingot"})}, {'#', Ingredient({"stick"})}});
    AddShaped("powered_rail", 6, {"X X", "X#X", "XRX"},
              {{'X', Ingredient({"gold_ingot"})}, {'#', Ingredient({"stick"})}, {'R', Ingredient({"redstone"})}});
    AddShaped("detector_rail", 6, {"X X", "X#X", "XRX"},
              {{'X', Ingredient({"iron_ingot"})}, {'#', Ingredient({"stone_pressure_plate"})}, {'R', Ingredient({"redstone"})}});
    AddShaped("minecart", 1, {"# #", "###"}, {{'#', Ingredient({"iron_ingot"})}});
    AddShapeless("chest_minecart", 1, {Ingredient({"chest"}), Ingredient({"minecart"})});
    AddShapeless("furnace_minecart", 1, {Ingredient({"furnace"}), Ingredient({"minecart"})});
    AddShapeless("tnt_minecart", 1, {Ingredient({"tnt"}), Ingredient({"minecart"})});
    AddShapeless("hopper_minecart", 1, {Ingredient({"hopper"}), Ingredient({"minecart"})});
    AddShaped("hopper", 1, {"I I", "ICI", " I "}, {{'I', Ingredient({"iron_ingot"})}, {'C', Ingredient({"chest"})}});
    AddShaped("dispenser", 1, {"###", "#X#", "#R#"}, {{'#', cobblestone}, {'X', Ingredient({"bow"})}, {'R', Ingredient({"redstone"})}});
    AddShaped("dropper", 1, {"###", "# #", "#R#"}, {{'#', cobblestone}, {'R', Ingredient({"redstone"})}});
    AddShaped("slime_block", 1, {"###", "###", "###"}, {{'#', Ingredient({"slime_ball"})}});
    AddShapeless("slime_ball", 9, {Ingredient({"slime_block"})});
    AddShaped("sticky_piston", 1, {"S", "P"}, {{'S', Ingredient({"slime_ball"})}, {'P', Ingredient({"piston"})}});
    AddShaped("redstone_torch", 1, {"X", "#"}, {{'X', Ingredient({"redstone"})}, {'#', Ingredient({"stick"})}});
    AddShaped("polished_echo_quartz", 4, {"##", "##"}, {{'#', Ingredient({"echo_quartz"})}});
    AddShaped("echo_quartz_bricks", 4, {"##", "##"}, {{'#', Ingredient({"polished_echo_quartz"})}});
    AddShaped("echo_quartz_pillar", 2, {"#", "#"}, {{'#', Ingredient({"echo_quartz"})}});
    AddShaped("resonator", 1, {"###", "#S#", "OOO"},
              {{'#', Ingredient({"polished_echo_quartz"})}, {'S', Ingredient({"resonant_shard"})}, {'O', Ingredient({"obsidian"})}});
    AddShaped("anvil", 1, {"III", " i ", "iii"}, {{'I', Ingredient({"iron_block"})}, {'i', Ingredient({"iron_ingot"})}});
    AddShaped("stone_pressure_plate", 1, {"##"}, {{'#', stone}});
    AddShaped("light_weighted_pressure_plate", 1, {"##"}, {{'#', Ingredient({"gold_ingot"})}});
    AddShaped("heavy_weighted_pressure_plate", 1, {"##"}, {{'#', Ingredient({"iron_ingot"})}});
    AddShaped("comparator", 1, {" # ", "#X#", "III"}, {{'#', Ingredient({"redstone_torch"})}, {'X', Ingredient({"quartz"})}, {'I', stone}});
    AddShaped("observer", 1, {"###", "RRQ", "###"}, {{'#', cobblestone}, {'R', Ingredient({"redstone"})}, {'Q', Ingredient({"quartz"})}});
    AddShaped("repeater", 1, {"#X#", "III"}, {{'#', Ingredient({"redstone_torch"})}, {'X', Ingredient({"redstone"})}, {'I', stone}});
    AddShaped("polished_granite", 4, {"##", "##"}, {{'#', Ingredient({"granite"})}});
    AddShaped("polished_diorite", 4, {"##", "##"}, {{'#', Ingredient({"diorite"})}});
    AddShaped("polished_andesite", 4, {"##", "##"}, {{'#', Ingredient({"andesite"})}});
    AddShapeless("andesite", 2, {Ingredient({"diorite"}), cobblestone});
    AddShaped("sandstone", 1, {"##", "##"}, {{'#', Ingredient({"sand"})}});
    AddShaped("red_sandstone", 1, {"##", "##"}, {{'#', Ingredient({"red_sand"})}});
    AddShaped("cut_sandstone", 4, {"##", "##"}, {{'#', Ingredient({"sandstone"})}});
    AddShaped("cut_red_sandstone", 4, {"##", "##"}, {{'#', Ingredient({"red_sandstone"})}});
    AddShaped("cobblestone_slab", 6, {"###"}, {{'#', cobblestone}});
    AddShaped("cobblestone_stairs", 4, {"#  ", "## ", "###"}, {{'#', cobblestone}});
    AddShaped("cobblestone_wall", 6, {"###", "###"}, {{'#', cobblestone}});
    AddShaped("mossy_cobblestone_wall", 6, {"###", "###"}, {{'#', Ingredient({"mossy_cobblestone"})}});
    AddShaped("smooth_stone_slab", 6, {"###"}, {{'#', Ingredient({"smooth_stone"})}});
    AddShaped("smooth_stone_stairs", 4, {"#  ", "## ", "###"}, {{'#', Ingredient({"smooth_stone"})}});
    AddShaped("sandstone_slab", 6, {"###"}, {{'#', Ingredient({"sandstone", "chiseled_sandstone"})}});
    AddShaped("sandstone_stairs", 4, {"#  ", "## ", "###"}, {{'#', Ingredient({"sandstone", "chiseled_sandstone"})}});
    AddShaped("red_sandstone_slab", 6, {"###"}, {{'#', Ingredient({"red_sandstone", "chiseled_red_sandstone"})}});
    AddShaped("red_sandstone_stairs", 4, {"#  ", "## ", "###"}, {{'#', Ingredient({"red_sandstone", "chiseled_red_sandstone"})}});
    AddShaped("chiseled_sandstone", 1, {"#", "#"}, {{'#', Ingredient({"sandstone_slab"})}});
    AddShaped("chiseled_red_sandstone", 1, {"#", "#"}, {{'#', Ingredient({"red_sandstone_slab"})}});
    AddShaped("dripstone_block", 1, {"##", "##"}, {{'#', Ingredient({"pointed_dripstone"})}});
}

void Recipes::AddToolRecipes()
{
    constexpr const char *kToolMaterials[]      = {"wooden", "stone", "iron", "golden", "diamond"};
    const std::vector<std::string> toolInputs[] = {
            {"oak_planks", "spruce_planks", "birch_planks", "acacia_planks", "jungle_planks", "cherry_planks", "dark_oak_planks", "bamboo_planks"},
            {"cobblestone"},
            {"iron_ingot"},
            {"gold_ingot"},
            {"diamond"}};
    Ingredient stick({"stick"});
    for (size_t i = 0; i < std::size(kToolMaterials); i++)
    {
        std::string name = kToolMaterials[i];
        Ingredient material(toolInputs[i]);
        AddShaped(name + "_pickaxe", 1, {"XXX", " # ", " # "}, {{'X', material}, {'#', stick}});
        AddShaped(name + "_axe", 1, {"XX", "X#", " #"}, {{'X', material}, {'#', stick}});
        AddShaped(name + "_shovel", 1, {"X", "#", "#"}, {{'X', material}, {'#', stick}});
        AddShaped(name + "_sword", 1, {"X", "X", "#"}, {{'X', material}, {'#', stick}});
        AddShaped(name + "_hoe", 1, {"XX", " #", " #"}, {{'X', material}, {'#', stick}});
    }

    constexpr const char *kArmorMaterials[] = {"leather", "chainmail", "iron", "golden", "diamond"};
    constexpr const char *kArmorInputs[]    = {"leather", "fire", "iron_ingot", "gold_ingot", "diamond"};
    for (size_t i = 0; i < std::size(kArmorMaterials); i++)
    {
        std::string name = kArmorMaterials[i];
        Ingredient material({kArmorInputs[i]});
        AddShaped(name + "_helmet", 1, {"XXX", "X X"}, {{'X', material}});
        AddShaped(name + "_chestplate", 1, {"X X", "XXX", "XXX"}, {{'X', material}});
        AddShaped(name + "_leggings", 1, {"XXX", "X X", "X X"}, {{'X', material}});
        AddShaped(name + "_boots", 1, {"X X", "X X"}, {{'X', material}});
    }

    Ingredient iron({"iron_ingot"});
    AddShaped("bucket", 1, {"# #", " # "}, {{'#', iron}});
    AddShaped("shears", 1, {" #", "# "}, {{'#', iron}});
    AddShaped("iron_ingot", 1, {"###", "###", "###"}, {{'#', Ingredient({"iron_nugget"})}});
    AddShapeless("iron_nugget", 9, {iron});
    const char *nuggetMaterials[][2] = {{"gold_ingot", "gold_nugget"},
                                        {"diamond", "diamond_nugget"},
                                        {"emerald", "emerald_nugget"},
                                        {"severite", "severite_nugget"}};
    for (size_t i = 0; i < std::size(nuggetMaterials); i++)
    {
        AddShaped(nuggetMaterials[i][0], 1, {"###", "###", "###"}, {{'#', Ingredient({nuggetMaterials[i][1]})}});
        AddShapeless(nuggetMaterials[i][1], 9, {Ingredient({nuggetMaterials[i][0]})});
    }

    AddShaped("golden_carrot", 1, {"###", "#C#", "###"}, {{'#', Ingredient({"gold_nugget"})}, {'C', Ingredient({"carrot"})}});
    AddShaped("orange_lantern", 1, {"###", "#T#", "###"}, {{'#', Ingredient({"iron_nugget"})}, {'T', Ingredient({"torch"})}});
    AddShaped("chain", 1, {"N", "I", "N"}, {{'N', Ingredient({"iron_nugget"})}, {'I', iron}});
    AddShaped("iron_bars", 16, {"###", "###"}, {{'#', iron}});
    AddShapeless("flint_and_steel", 1, {iron, Ingredient({"flint"})});

    AddShaped("bow", 1, {" #X", "# X", " #X"}, {{'#', stick}, {'X', Ingredient({"string"})}});
    AddShaped("arrow", 4, {"X", "#", "Y"}, {{'X', Ingredient({"flint"})}, {'#', stick}, {'Y', Ingredient({"feather"})}});
}

void Recipes::AddDyeRecipes()
{
    AddShapeless("blue_dye", 1, {Ingredient({"lapis_lazuli"})});
    AddShapeless("red_dye", 2, {Ingredient({"rose_bush"})});
    AddShapeless("red_dye", 1, {Ingredient({"beetroot"})});
    AddShapeless("magenta_dye", 2, {Ingredient({"lilac"})});
    AddShapeless("pink_dye", 2, {Ingredient({"peony"})});
    AddShapeless("yellow_dye", 1, {Ingredient({"dandelion"})});
    AddShapeless("yellow_dye", 2, {Ingredient({"sunbloom"})});
    AddShapeless("red_dye", 1, {Ingredient({"poppy", "red_tulip"})});
    AddShapeless("light_blue_dye", 1, {Ingredient({"blue_orchid"})});
    AddShapeless("magenta_dye", 1, {Ingredient({"allium"})});
    AddShapeless("light_gray_dye", 1, {Ingredient({"azure_bluet", "white_tulip", "oxeye_daisy"})});
    AddShapeless("orange_dye", 1, {Ingredient({"orange_tulip"})});
    AddShapeless("pink_dye", 1, {Ingredient({"pink_tulip"})});

    Ingredient white({"bone_meal"});
    Ingredient red({"red_dye"});
    Ingredient blue({"blue_dye"});
    Ingredient black({"black_dye"});
    Ingredient green({"green_dye"});
    AddShapeless("orange_dye", 2, {red, Ingredient({"yellow_dye"})});
    AddShapeless("light_blue_dye", 2, {blue, white});
    AddShapeless("lime_dye", 2, {green, white});
    AddShapeless("pink_dye", 2, {red, white});
    AddShapeless("gray_dye", 2, {black, white});
    AddShapeless("light_gray_dye", 2, {Ingredient({"gray_dye"}), white});
    AddShapeless("light_gray_dye", 3, {black, white, white});
    AddShapeless("cyan_dye", 2, {blue, green});
    AddShapeless("purple_dye", 2, {blue, red});
    AddShapeless("magenta_dye", 2, {Ingredient({"purple_dye"}), Ingredient({"pink_dye"})});
    AddShapeless("magenta_dye", 3, {blue, red, Ingredient({"pink_dye"})});
    AddShapeless("magenta_dye", 4, {blue, red, red, white});

    constexpr const char *kColors[] = {"white", "light_gray", "gray", "black",      "brown", "red",    "orange",  "yellow",
                                       "lime",  "green",      "cyan", "light_blue", "blue",  "purple", "magenta", "pink"};
    std::vector<std::string> allWool;
    for (size_t i = 0; i < std::size(kColors); i++)
    {
        allWool.push_back(std::string(kColors[i]) + "_wool");
    }

    AddShaped("white_wool", 1, {"##", "##"}, {{'#', Ingredient({"string"})}});
    Ingredient planks(
            {"oak_planks", "spruce_planks", "birch_planks", "acacia_planks", "jungle_planks", "cherry_planks", "dark_oak_planks", "bamboo_planks"});
    for (size_t i = 0; i < std::size(kColors); i++)
    {
        std::string color = kColors[i];
        Ingredient dye({color == "white" ? "bone_meal" : (color == "brown" ? "cocoa_beans" : color + "_dye")});
        std::vector<std::string> otherWool;
        for (size_t j = 0; j < allWool.size(); j++)
        {
            if (j != i)
            {
                otherWool.push_back(allWool[j]);
            }
        }

        AddShapeless(color + "_wool", 1, {dye, Ingredient(otherWool)});
        AddShaped(color + "_stained_glass", 8, {"###", "#X#", "###"}, {{'#', Ingredient({"glass"})}, {'X', dye}});
        AddShaped(color + "_stained_glass_pane", 16, {"###", "###"}, {{'#', Ingredient({color + "_stained_glass"})}});
        AddShaped(color + "_bed", 1, {"###", "XXX"}, {{'#', Ingredient({allWool[i]})}, {'X', planks}});
        AddShaped(color + "_carpet", 3, {"##"}, {{'#', Ingredient({allWool[i]})}});

        std::vector<std::string> otherLanterns;
        for (size_t j = 0; j < std::size(kColors); j++)
        {
            if (j != i)
            {
                otherLanterns.push_back(std::string(kColors[j]) + "_lantern");
            }
        }

        AddShapeless(color + "_lantern", 1, {dye, Ingredient(otherLanterns)});
    }
}

void Recipes::AddMiscRecipes()
{
    Ingredient beetroot({"beetroot"});
    AddShaped("glass_pane", 16, {"###", "###"}, {{'#', Ingredient({"glass"})}});
    AddShaped("glass", 1, {"##", "##"}, {{'#', Ingredient({"glass_shard"})}});
    AddShaped("ice", 1, {"##", "##"}, {{'#', Ingredient({"ice_shard"})}});
    AddShaped("ice_sheet", 4, {"#I#"}, {{'#', Ingredient({"ice_shard"})}, {'I', Ingredient({"ice"})}});
    AddShaped("snow_block", 1, {"##", "##"}, {{'#', Ingredient({"snowball"})}});
    AddShaped("snow", 6, {"###"}, {{'#', Ingredient({"snow_block"})}});
    AddShaped("clay", 1, {"##", "##"}, {{'#', Ingredient({"clay_ball"})}});
    AddShaped("paper", 3, {"###"}, {{'#', Ingredient({"sugar_cane"})}});
    AddShapeless("book", 1, {Ingredient({"paper"}), Ingredient({"paper"}), Ingredient({"paper"}), Ingredient({"leather"})});
    AddShaped("lead", 2, {"~~ ", "~L ", "  ~"}, {{'~', Ingredient({"string"})}, {'L', Ingredient({"leather"})}});
    const char *storageBlocks[]    = {"coal_block", "iron_block", "gold_block", "diamond_block", "emerald_block", "lapis_block", "redstone_block"};
    const char *storageMaterials[] = {"coal", "iron_ingot", "gold_ingot", "diamond", "emerald", "lapis_lazuli", "redstone"};
    for (size_t i = 0; i < std::size(storageBlocks); i++)
    {
        AddShaped(storageBlocks[i], 1, {"###", "###", "###"}, {{'#', Ingredient({storageMaterials[i]})}});
        AddShapeless(storageMaterials[i], 9, {Ingredient({storageBlocks[i]})});
    }

    AddShaped("packed_ice", 1, {"###", "###", "###"}, {{'#', Ingredient({"ice"})}});
    AddShaped("bread", 1, {"###"}, {{'#', Ingredient({"wheat"})}});
    AddShaped("hay_block", 1, {"###", "###", "###"}, {{'#', Ingredient({"wheat"})}});
    AddShapeless("wheat", 9, {Ingredient({"hay_block"})});
    AddShaped("melon", 1, {"###", "###", "###"}, {{'#', Ingredient({"melon_slice"})}});
    AddShapeless("pumpkin_seeds", 4, {Ingredient({"pumpkin"})});
    AddShaped("fracture_core", 1, {"OSO", "SDS", "OSO"},
              {{'O', Ingredient({"obsidian"})}, {'S', Ingredient({"resonant_shard"})}, {'D', Ingredient({"diamond_block"})}});
    AddShaped("tnt", 1, {"X#X", "#X#", "X#X"}, {{'X', Ingredient({"gunpowder"})}, {'#', Ingredient({"sand", "red_sand"})}});
    AddShaped("coarse_dirt", 4, {"DG", "GD"}, {{'D', Ingredient({"dirt"})}, {'G', Ingredient({"gravel"})}});
    AddShapeless("beetroot_soup", 1, {Ingredient({"bowl"}), beetroot, beetroot, beetroot, beetroot, beetroot, beetroot});
    AddShapeless("melon_seeds", 1, {Ingredient({"melon_slice"})});
    AddShapeless("bone_meal", 3, {Ingredient({"bone"})});
}

void Recipes::AddDecorationRecipes()
{
    constexpr const char *kColors[] = {"white", "light_gray", "gray", "black",      "brown", "red",    "orange",  "yellow",
                                       "lime",  "green",      "cyan", "light_blue", "blue",  "purple", "magenta", "pink"};
    Ingredient clay({"clay"});
    for (size_t i = 0; i < std::size(kColors); i++)
    {
        std::string color = kColors[i];
        Ingredient wool({color + "_wool"});
        Ingredient bricks({color + "_bricks"});
        AddShaped(color + "_wool_slab", 6, {"###"}, {{'#', wool}});
        AddShaped(color + "_wool_stairs", 4, {"#  ", "## ", "###"}, {{'#', wool}});
        AddShaped(color + "_bricks", 8, {"###", "#X#", "###"},
                  {{'#', clay}, {'X', Ingredient({color == "white" ? "bone_meal" : (color == "brown" ? "cocoa_beans" : color + "_dye")})}});
        AddShaped(color + "_brick_slab", 6, {"###"}, {{'#', bricks}});
        AddShaped(color + "_brick_stairs", 4, {"#  ", "## ", "###"}, {{'#', bricks}});
    }

    Ingredient vibrantRedBricks({"vibrant_red_bricks"});
    AddShapeless("vibrant_red_bricks", 1, {Ingredient({"red_bricks"}), Ingredient({"red_dye"})});
    AddShaped("vibrant_red_brick_slab", 6, {"###"}, {{'#', vibrantRedBricks}});
    AddShaped("vibrant_red_brick_stairs", 4, {"#  ", "## ", "###"}, {{'#', vibrantRedBricks}});

    Ingredient terracotta({"terracotta"});
    for (std::string color : {"white", "orange", "yellow", "light_gray", "brown", "red", "magenta", "light_blue", "lime", "pink", "gray", "cyan",
                              "purple", "blue", "green", "black"})
    {
        AddShaped(color + "_terracotta", 8, {"###", "#X#", "###"},
                  {{'#', terracotta}, {'X', Ingredient({color == "white" ? "bone_meal" : (color == "brown" ? "cocoa_beans" : color + "_dye")})}});
    }

    Ingredient crackedStoneBricks({"cracked_stone_bricks"});
    AddShaped("cracked_stone_brick_slab", 6, {"###"}, {{'#', crackedStoneBricks}});
    AddShaped("cracked_stone_brick_stairs", 4, {"#  ", "## ", "###"}, {{'#', crackedStoneBricks}});

    constexpr const char *kLampColors[] = {"red", "yellow", "green", "cyan", "blue", "purple", "pink"};
    Ingredient glass({"glass"});
    Ingredient torch({"torch"});
    for (const char *color : kLampColors)
    {
        AddShaped(std::string(color) + "_lamp", 1, {" X ", "#T#", " # "},
                  {{'#', glass}, {'T', torch}, {'X', Ingredient({std::string(color) + "_dye"})}});
    }

    AddShaped("white_lamp", 1, {" X ", "#T#", " # "}, {{'#', glass}, {'T', torch}, {'X', Ingredient({"bone_meal"})}});
    AddShaped("redstone_lamp", 1, {"#R#", "RTR", "#R#"}, {{'#', glass}, {'R', Ingredient({"redstone"})}, {'T', torch}});

    AddShaped("checkered_tiles", 4, {"#X", "X#"}, {{'#', Ingredient({"polished_diorite"})}, {'X', Ingredient({"black_dye"})}});
}
