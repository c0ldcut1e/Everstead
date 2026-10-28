#include "ItemRegistry.h"

#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../entity/animal/Chicken.h"
#include "../../entity/animal/Cow.h"
#include "../../entity/animal/Ocelot.h"
#include "../../entity/animal/Parrot.h"
#include "../../entity/animal/Pig.h"
#include "../../entity/animal/Sheep.h"
#include "../../entity/animal/Squid.h"
#include "../../entity/item/Boat.h"
#include "../../entity/item/Minecart.h"
#include "../../entity/monster/Creeper.h"
#include "../../entity/monster/Skeleton.h"
#include "../../entity/monster/Slime.h"
#include "../../entity/monster/Spider.h"
#include "../../entity/monster/Zombie.h"
#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/BlockRegistry.h"
#include "ArmorItem.h"
#include "BlockItem.h"
#include "BoatItem.h"
#include "BoneMealItem.h"
#include "BowItem.h"
#include "BucketItem.h"
#include "DiggerItem.h"
#include "EchoSeedItem.h"
#include "EggItem.h"
#include "FlintAndSteelItem.h"
#include "FoodItem.h"
#include "HoeItem.h"
#include "MilkBucketItem.h"
#include "MinecartItem.h"
#include "SeedItem.h"
#include "ShearsItem.h"
#include "ShovelItem.h"
#include "SnowballItem.h"
#include "SpawnEggItem.h"
#include "SwordItem.h"

ItemRegistry *ItemRegistry::GetInstance()
{
    static ItemRegistry instance;
    return &instance;
}

static void RegisterBlockItem(ItemRegistry *registry, std::vector<std::unique_ptr<BlockItem>> *items, const ResourceLocation &blockName,
                              const std::string &iconPath = "")
{
    Block *block = Block::ByName(blockName);
    if (!block)
    {
        return;
    }

    std::unique_ptr<BlockItem> item = std::make_unique<BlockItem>(block);
    item->SetIconPath(iconPath.empty() ? block->GetTexturePath(Direction::north) : iconPath);
    item->SetId(registry->Register(blockName, item.get()));
    items->push_back(std::move(item));
}

void ItemRegistry::Init()
{
    ItemRegistry *registry = GetInstance();

    static Item stick;
    stick.SetIconPath("assets/texture/item/stick.png");
    stick.SetHandEquipped(true);
    stick.SetId(registry->Register("stick", &stick));

    static Item compass;
    compass.SetIconPath("assets/texture/item/compass.png");
    compass.SetId(registry->Register("compass", &compass));

    static BoneMealItem boneMeal;
    boneMeal.SetIconPath("assets/texture/item/bone_meal.png");
    boneMeal.SetId(registry->Register("bone_meal", &boneMeal));

    static EchoSeedItem echoSeed;
    echoSeed.SetIconPath("assets/texture/item/echo_seed.png");
    echoSeed.SetId(registry->Register("echo_seed", &echoSeed));

    static std::vector<std::unique_ptr<BlockItem>> blockItems;
    RegisterBlockItem(registry, &blockItems, "bedrock");
    RegisterBlockItem(registry, &blockItems, "stone");
    RegisterBlockItem(registry, &blockItems, "cobblestone");
    RegisterBlockItem(registry, &blockItems, "mossy_cobblestone");
    RegisterBlockItem(registry, &blockItems, "moss_block");
    RegisterBlockItem(registry, &blockItems, "moss_carpet");
    RegisterBlockItem(registry, &blockItems, "obsidian");
    RegisterBlockItem(registry, &blockItems, "andesite");
    RegisterBlockItem(registry, &blockItems, "granite");
    RegisterBlockItem(registry, &blockItems, "polished_granite");
    RegisterBlockItem(registry, &blockItems, "diorite");
    RegisterBlockItem(registry, &blockItems, "polished_diorite");
    RegisterBlockItem(registry, &blockItems, "polished_andesite");
    RegisterBlockItem(registry, &blockItems, "smooth_stone");
    RegisterBlockItem(registry, &blockItems, "smooth_stone_slab");
    RegisterBlockItem(registry, &blockItems, "smooth_stone_stairs");
    RegisterBlockItem(registry, &blockItems, "stone_bricks");
    RegisterBlockItem(registry, &blockItems, "mossy_stone_bricks");
    RegisterBlockItem(registry, &blockItems, "cracked_stone_bricks");
    RegisterBlockItem(registry, &blockItems, "chiseled_stone_bricks");
    RegisterBlockItem(registry, &blockItems, "stone_slab");
    RegisterBlockItem(registry, &blockItems, "stone_stairs");
    RegisterBlockItem(registry, &blockItems, "stone_brick_slab");
    RegisterBlockItem(registry, &blockItems, "stone_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "dirt");
    RegisterBlockItem(registry, &blockItems, "coarse_dirt");
    RegisterBlockItem(registry, &blockItems, "podzol");
    RegisterBlockItem(registry, &blockItems, "grass_block");
    RegisterBlockItem(registry, &blockItems, "grass");
    RegisterBlockItem(registry, &blockItems, "fern");
    RegisterBlockItem(registry, &blockItems, "sand");
    RegisterBlockItem(registry, &blockItems, "sand_layer");
    RegisterBlockItem(registry, &blockItems, "gravel");
    RegisterBlockItem(registry, &blockItems, "clay");
    RegisterBlockItem(registry, &blockItems, "sandstone");
    RegisterBlockItem(registry, &blockItems, "chiseled_sandstone");
    RegisterBlockItem(registry, &blockItems, "cut_sandstone");
    RegisterBlockItem(registry, &blockItems, "smooth_sandstone");
    RegisterBlockItem(registry, &blockItems, "red_sandstone");
    RegisterBlockItem(registry, &blockItems, "chiseled_red_sandstone");
    RegisterBlockItem(registry, &blockItems, "cut_red_sandstone");
    RegisterBlockItem(registry, &blockItems, "smooth_red_sandstone");
    RegisterBlockItem(registry, &blockItems, "red_sand");
    RegisterBlockItem(registry, &blockItems, "cactus");
    RegisterBlockItem(registry, &blockItems, "pumpkin", "assets/texture/block/pumpkin_side.png");
    RegisterBlockItem(registry, &blockItems, "carved_pumpkin");
    RegisterBlockItem(registry, &blockItems, "melon", "assets/texture/block/melon_side.png");
    RegisterBlockItem(registry, &blockItems, "dead_bush");
    RegisterBlockItem(registry, &blockItems, "sugar_cane", "assets/texture/item/sugar_cane.png");
    RegisterBlockItem(registry, &blockItems, "tall_grass", "assets/texture/block/tall_grass_top.png");
    RegisterBlockItem(registry, &blockItems, "large_fern", "assets/texture/block/large_fern_top.png");
    RegisterBlockItem(registry, &blockItems, "lilac", "assets/texture/block/lilac_top.png");
    RegisterBlockItem(registry, &blockItems, "rose_bush", "assets/texture/block/rose_bush_top.png");
    RegisterBlockItem(registry, &blockItems, "peony", "assets/texture/block/peony_top.png");
    RegisterBlockItem(registry, &blockItems, "dandelion");
    RegisterBlockItem(registry, &blockItems, "poppy");
    RegisterBlockItem(registry, &blockItems, "blue_orchid");
    RegisterBlockItem(registry, &blockItems, "allium");
    RegisterBlockItem(registry, &blockItems, "azure_bluet");
    RegisterBlockItem(registry, &blockItems, "red_tulip");
    RegisterBlockItem(registry, &blockItems, "orange_tulip");
    RegisterBlockItem(registry, &blockItems, "white_tulip");
    RegisterBlockItem(registry, &blockItems, "pink_tulip");
    RegisterBlockItem(registry, &blockItems, "oxeye_daisy");
    RegisterBlockItem(registry, &blockItems, "sunbloom");
    RegisterBlockItem(registry, &blockItems, "oak_sapling");
    RegisterBlockItem(registry, &blockItems, "spruce_sapling");
    RegisterBlockItem(registry, &blockItems, "birch_sapling");
    RegisterBlockItem(registry, &blockItems, "acacia_sapling");
    RegisterBlockItem(registry, &blockItems, "jungle_sapling");
    RegisterBlockItem(registry, &blockItems, "cherry_sapling");
    RegisterBlockItem(registry, &blockItems, "prickly_vine");
    RegisterBlockItem(registry, &blockItems, "snow_block");
    RegisterBlockItem(registry, &blockItems, "snow");
    RegisterBlockItem(registry, &blockItems, "mossy_stone_brick_slab");
    RegisterBlockItem(registry, &blockItems, "mossy_stone_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "cracked_stone_brick_slab");
    RegisterBlockItem(registry, &blockItems, "cracked_stone_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "white_brick_slab");
    RegisterBlockItem(registry, &blockItems, "white_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "light_gray_brick_slab");
    RegisterBlockItem(registry, &blockItems, "light_gray_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "gray_brick_slab");
    RegisterBlockItem(registry, &blockItems, "gray_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "black_brick_slab");
    RegisterBlockItem(registry, &blockItems, "black_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "brown_brick_slab");
    RegisterBlockItem(registry, &blockItems, "brown_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "red_brick_slab");
    RegisterBlockItem(registry, &blockItems, "red_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "vibrant_red_brick_slab");
    RegisterBlockItem(registry, &blockItems, "vibrant_red_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "orange_brick_slab");
    RegisterBlockItem(registry, &blockItems, "orange_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "yellow_brick_slab");
    RegisterBlockItem(registry, &blockItems, "yellow_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "lime_brick_slab");
    RegisterBlockItem(registry, &blockItems, "lime_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "green_brick_slab");
    RegisterBlockItem(registry, &blockItems, "green_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "cyan_brick_slab");
    RegisterBlockItem(registry, &blockItems, "cyan_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "light_blue_brick_slab");
    RegisterBlockItem(registry, &blockItems, "light_blue_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "blue_brick_slab");
    RegisterBlockItem(registry, &blockItems, "blue_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "purple_brick_slab");
    RegisterBlockItem(registry, &blockItems, "purple_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "magenta_brick_slab");
    RegisterBlockItem(registry, &blockItems, "magenta_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "pink_brick_slab");
    RegisterBlockItem(registry, &blockItems, "pink_brick_stairs");
    RegisterBlockItem(registry, &blockItems, "white_wool_slab");
    RegisterBlockItem(registry, &blockItems, "white_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "light_gray_wool_slab");
    RegisterBlockItem(registry, &blockItems, "light_gray_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "gray_wool_slab");
    RegisterBlockItem(registry, &blockItems, "gray_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "black_wool_slab");
    RegisterBlockItem(registry, &blockItems, "black_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "brown_wool_slab");
    RegisterBlockItem(registry, &blockItems, "brown_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "red_wool_slab");
    RegisterBlockItem(registry, &blockItems, "red_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "orange_wool_slab");
    RegisterBlockItem(registry, &blockItems, "orange_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "yellow_wool_slab");
    RegisterBlockItem(registry, &blockItems, "yellow_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "lime_wool_slab");
    RegisterBlockItem(registry, &blockItems, "lime_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "green_wool_slab");
    RegisterBlockItem(registry, &blockItems, "green_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "cyan_wool_slab");
    RegisterBlockItem(registry, &blockItems, "cyan_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "light_blue_wool_slab");
    RegisterBlockItem(registry, &blockItems, "light_blue_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "blue_wool_slab");
    RegisterBlockItem(registry, &blockItems, "blue_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "purple_wool_slab");
    RegisterBlockItem(registry, &blockItems, "purple_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "magenta_wool_slab");
    RegisterBlockItem(registry, &blockItems, "magenta_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "pink_wool_slab");
    RegisterBlockItem(registry, &blockItems, "pink_wool_stairs");
    RegisterBlockItem(registry, &blockItems, "water_still");
    RegisterBlockItem(registry, &blockItems, "lava_still");
    RegisterBlockItem(registry, &blockItems, "water_flow");
    RegisterBlockItem(registry, &blockItems, "lava_flow");
    RegisterBlockItem(registry, &blockItems, "oak_log");
    RegisterBlockItem(registry, &blockItems, "spruce_log");
    RegisterBlockItem(registry, &blockItems, "oak_planks");
    RegisterBlockItem(registry, &blockItems, "spruce_planks");
    RegisterBlockItem(registry, &blockItems, "oak_slab");
    RegisterBlockItem(registry, &blockItems, "oak_stairs");
    RegisterBlockItem(registry, &blockItems, "spruce_slab");
    RegisterBlockItem(registry, &blockItems, "spruce_stairs");
    RegisterBlockItem(registry, &blockItems, "birch_slab");
    RegisterBlockItem(registry, &blockItems, "birch_stairs");
    RegisterBlockItem(registry, &blockItems, "acacia_slab");
    RegisterBlockItem(registry, &blockItems, "jungle_slab");
    RegisterBlockItem(registry, &blockItems, "acacia_stairs");
    RegisterBlockItem(registry, &blockItems, "jungle_stairs");
    RegisterBlockItem(registry, &blockItems, "cherry_slab");
    RegisterBlockItem(registry, &blockItems, "cherry_stairs");
    RegisterBlockItem(registry, &blockItems, "bamboo_slab");
    RegisterBlockItem(registry, &blockItems, "bamboo_stairs");
    RegisterBlockItem(registry, &blockItems, "bamboo_mosaic_slab");
    RegisterBlockItem(registry, &blockItems, "bamboo_mosaic_stairs");
    RegisterBlockItem(registry, &blockItems, "cobblestone_slab");
    RegisterBlockItem(registry, &blockItems, "cobblestone_stairs");
    RegisterBlockItem(registry, &blockItems, "sandstone_slab");
    RegisterBlockItem(registry, &blockItems, "sandstone_stairs");
    RegisterBlockItem(registry, &blockItems, "red_sandstone_slab");
    RegisterBlockItem(registry, &blockItems, "red_sandstone_stairs");
    RegisterBlockItem(registry, &blockItems, "terracotta");
    RegisterBlockItem(registry, &blockItems, "hay_block");
    RegisterBlockItem(registry, &blockItems, "oak_leaves");
    RegisterBlockItem(registry, &blockItems, "spruce_leaves");
    RegisterBlockItem(registry, &blockItems, "crafting_table");
    RegisterBlockItem(registry, &blockItems, "furnace");
    RegisterBlockItem(registry, &blockItems, "chest");
    const char *colorNames[] = {"white", "light_gray", "gray", "black",      "brown", "red",    "orange",  "yellow",
                                "lime",  "green",      "cyan", "light_blue", "blue",  "purple", "magenta", "pink"};
    for (size_t i = 0; i < std::size(colorNames); i++)
    {
        std::string color = colorNames[i];
        RegisterBlockItem(registry, &blockItems, color + "_bed", "assets/texture/item/" + color + "_bed.png");
        Item::ByName(color + "_bed")->SetMaxStackSize(1);
        RegisterBlockItem(registry, &blockItems, color + "_carpet");
    }
    RegisterBlockItem(registry, &blockItems, "mob_spawner");
    RegisterBlockItem(registry, &blockItems, "tnt");
    RegisterBlockItem(registry, &blockItems, "torch");
    RegisterBlockItem(registry, &blockItems, "torch_wall");
    for (size_t i = 0; i < std::size(colorNames); i++)
    {
        std::string color = colorNames[i];
        RegisterBlockItem(registry, &blockItems, color + "_lantern", "assets/texture/item/" + color + "_lantern.png");
    }

    RegisterBlockItem(registry, &blockItems, "chain", "assets/texture/item/chain.png");
    RegisterBlockItem(registry, &blockItems, "iron_bars");
    RegisterBlockItem(registry, &blockItems, "white_lamp");
    RegisterBlockItem(registry, &blockItems, "red_lamp");
    RegisterBlockItem(registry, &blockItems, "yellow_lamp");
    RegisterBlockItem(registry, &blockItems, "green_lamp");
    RegisterBlockItem(registry, &blockItems, "cyan_lamp");
    RegisterBlockItem(registry, &blockItems, "blue_lamp");
    RegisterBlockItem(registry, &blockItems, "purple_lamp");
    RegisterBlockItem(registry, &blockItems, "pink_lamp");
    RegisterBlockItem(registry, &blockItems, "checkered_tiles");
    RegisterBlockItem(registry, &blockItems, "white_bricks");
    RegisterBlockItem(registry, &blockItems, "light_gray_bricks");
    RegisterBlockItem(registry, &blockItems, "gray_bricks");
    RegisterBlockItem(registry, &blockItems, "black_bricks");
    RegisterBlockItem(registry, &blockItems, "brown_bricks");
    RegisterBlockItem(registry, &blockItems, "red_bricks");
    RegisterBlockItem(registry, &blockItems, "vibrant_red_bricks");
    RegisterBlockItem(registry, &blockItems, "orange_bricks");
    RegisterBlockItem(registry, &blockItems, "yellow_bricks");
    RegisterBlockItem(registry, &blockItems, "lime_bricks");
    RegisterBlockItem(registry, &blockItems, "green_bricks");
    RegisterBlockItem(registry, &blockItems, "cyan_bricks");
    RegisterBlockItem(registry, &blockItems, "light_blue_bricks");
    RegisterBlockItem(registry, &blockItems, "blue_bricks");
    RegisterBlockItem(registry, &blockItems, "purple_bricks");
    RegisterBlockItem(registry, &blockItems, "magenta_bricks");
    RegisterBlockItem(registry, &blockItems, "pink_bricks");
    RegisterBlockItem(registry, &blockItems, "white_wool");
    RegisterBlockItem(registry, &blockItems, "light_gray_wool");
    RegisterBlockItem(registry, &blockItems, "gray_wool");
    RegisterBlockItem(registry, &blockItems, "black_wool");
    RegisterBlockItem(registry, &blockItems, "brown_wool");
    RegisterBlockItem(registry, &blockItems, "red_wool");
    RegisterBlockItem(registry, &blockItems, "orange_wool");
    RegisterBlockItem(registry, &blockItems, "yellow_wool");
    RegisterBlockItem(registry, &blockItems, "lime_wool");
    RegisterBlockItem(registry, &blockItems, "green_wool");
    RegisterBlockItem(registry, &blockItems, "cyan_wool");
    RegisterBlockItem(registry, &blockItems, "light_blue_wool");
    RegisterBlockItem(registry, &blockItems, "blue_wool");
    RegisterBlockItem(registry, &blockItems, "purple_wool");
    RegisterBlockItem(registry, &blockItems, "magenta_wool");
    RegisterBlockItem(registry, &blockItems, "pink_wool");
    RegisterBlockItem(registry, &blockItems, "birch_log");
    RegisterBlockItem(registry, &blockItems, "acacia_log");
    RegisterBlockItem(registry, &blockItems, "jungle_log");
    RegisterBlockItem(registry, &blockItems, "birch_planks");
    RegisterBlockItem(registry, &blockItems, "acacia_planks");
    RegisterBlockItem(registry, &blockItems, "jungle_planks");
    RegisterBlockItem(registry, &blockItems, "birch_leaves");
    RegisterBlockItem(registry, &blockItems, "acacia_leaves");
    RegisterBlockItem(registry, &blockItems, "jungle_leaves");
    RegisterBlockItem(registry, &blockItems, "cherry_log");
    RegisterBlockItem(registry, &blockItems, "cherry_planks");
    RegisterBlockItem(registry, &blockItems, "cherry_leaves");
    RegisterBlockItem(registry, &blockItems, "bamboo_block");
    RegisterBlockItem(registry, &blockItems, "bamboo_planks");
    RegisterBlockItem(registry, &blockItems, "bamboo_mosaic");
    RegisterBlockItem(registry, &blockItems, "pink_petals", "assets/texture/item/pink_petals.png");
    RegisterBlockItem(registry, &blockItems, "ladder");
    RegisterBlockItem(registry, &blockItems, "vine");
    RegisterBlockItem(registry, &blockItems, "oak_door", "assets/texture/item/door_wood.png");
    RegisterBlockItem(registry, &blockItems, "spruce_door", "assets/texture/item/door_spruce.png");
    RegisterBlockItem(registry, &blockItems, "birch_door", "assets/texture/item/door_birch.png");
    RegisterBlockItem(registry, &blockItems, "acacia_door", "assets/texture/item/door_acacia.png");
    RegisterBlockItem(registry, &blockItems, "jungle_door", "assets/texture/item/door_jungle.png");
    RegisterBlockItem(registry, &blockItems, "cherry_door", "assets/texture/item/door_cherry.png");
    RegisterBlockItem(registry, &blockItems, "bamboo_door", "assets/texture/item/door_bamboo.png");
    RegisterBlockItem(registry, &blockItems, "glass");
    RegisterBlockItem(registry, &blockItems, "oak_fence");
    RegisterBlockItem(registry, &blockItems, "oak_fence_gate");
    RegisterBlockItem(registry, &blockItems, "oak_button");
    RegisterBlockItem(registry, &blockItems, "oak_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "spruce_fence");
    RegisterBlockItem(registry, &blockItems, "spruce_fence_gate");
    RegisterBlockItem(registry, &blockItems, "spruce_button");
    RegisterBlockItem(registry, &blockItems, "spruce_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "birch_fence");
    RegisterBlockItem(registry, &blockItems, "birch_fence_gate");
    RegisterBlockItem(registry, &blockItems, "birch_button");
    RegisterBlockItem(registry, &blockItems, "birch_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "acacia_fence");
    RegisterBlockItem(registry, &blockItems, "jungle_fence");
    RegisterBlockItem(registry, &blockItems, "acacia_fence_gate");
    RegisterBlockItem(registry, &blockItems, "jungle_fence_gate");
    RegisterBlockItem(registry, &blockItems, "acacia_button");
    RegisterBlockItem(registry, &blockItems, "jungle_button");
    RegisterBlockItem(registry, &blockItems, "acacia_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "jungle_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "cherry_fence");
    RegisterBlockItem(registry, &blockItems, "cherry_fence_gate");
    RegisterBlockItem(registry, &blockItems, "cherry_button");
    RegisterBlockItem(registry, &blockItems, "cherry_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "bamboo_fence");
    RegisterBlockItem(registry, &blockItems, "bamboo_fence_gate");
    RegisterBlockItem(registry, &blockItems, "bamboo_button");
    RegisterBlockItem(registry, &blockItems, "bamboo_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "stone_button");
    RegisterBlockItem(registry, &blockItems, "lever");
    RegisterBlockItem(registry, &blockItems, "stone_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "glass_pane");
    RegisterBlockItem(registry, &blockItems, "white_stained_glass");
    RegisterBlockItem(registry, &blockItems, "white_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "light_gray_stained_glass");
    RegisterBlockItem(registry, &blockItems, "light_gray_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "gray_stained_glass");
    RegisterBlockItem(registry, &blockItems, "gray_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "black_stained_glass");
    RegisterBlockItem(registry, &blockItems, "black_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "brown_stained_glass");
    RegisterBlockItem(registry, &blockItems, "brown_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "red_stained_glass");
    RegisterBlockItem(registry, &blockItems, "red_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "orange_stained_glass");
    RegisterBlockItem(registry, &blockItems, "orange_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "yellow_stained_glass");
    RegisterBlockItem(registry, &blockItems, "yellow_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "lime_stained_glass");
    RegisterBlockItem(registry, &blockItems, "lime_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "green_stained_glass");
    RegisterBlockItem(registry, &blockItems, "green_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "cyan_stained_glass");
    RegisterBlockItem(registry, &blockItems, "cyan_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "light_blue_stained_glass");
    RegisterBlockItem(registry, &blockItems, "light_blue_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "blue_stained_glass");
    RegisterBlockItem(registry, &blockItems, "blue_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "purple_stained_glass");
    RegisterBlockItem(registry, &blockItems, "purple_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "magenta_stained_glass");
    RegisterBlockItem(registry, &blockItems, "magenta_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "pink_stained_glass");
    RegisterBlockItem(registry, &blockItems, "pink_stained_glass_pane");
    RegisterBlockItem(registry, &blockItems, "ice");
    RegisterBlockItem(registry, &blockItems, "packed_ice");
    RegisterBlockItem(registry, &blockItems, "dripstone_block");
    RegisterBlockItem(registry, &blockItems, "pointed_dripstone", "assets/texture/item/pointed_dripstone.png");
    RegisterBlockItem(registry, &blockItems, "enchanted_ice");
    RegisterBlockItem(registry, &blockItems, "resonant_crystal");
    RegisterBlockItem(registry, &blockItems, "sealed_fracture_frame");
    RegisterBlockItem(registry, &blockItems, "cracked_fracture_frame");
    RegisterBlockItem(registry, &blockItems, "fractured_fracture_frame");
    RegisterBlockItem(registry, &blockItems, "broken_fracture_frame");
    RegisterBlockItem(registry, &blockItems, "resonant_socket");
    RegisterBlockItem(registry, &blockItems, "fracture_core");
    RegisterBlockItem(registry, &blockItems, "echo_quartz");
    RegisterBlockItem(registry, &blockItems, "polished_echo_quartz");
    RegisterBlockItem(registry, &blockItems, "echo_quartz_bricks");
    RegisterBlockItem(registry, &blockItems, "echo_quartz_pillar");
    RegisterBlockItem(registry, &blockItems, "echo_glass");
    RegisterBlockItem(registry, &blockItems, "hollow_core");
    RegisterBlockItem(registry, &blockItems, "resonator");
    RegisterBlockItem(registry, &blockItems, "anvil");
    RegisterBlockItem(registry, &blockItems, "chipped_anvil");
    RegisterBlockItem(registry, &blockItems, "damaged_anvil");
    RegisterBlockItem(registry, &blockItems, "rich_resonant_crystal");
    RegisterBlockItem(registry, &blockItems, "effects_experiment");
    RegisterBlockItem(registry, &blockItems, "singularity_experiment");
    RegisterBlockItem(registry, &blockItems, "nova_experiment");
    RegisterBlockItem(registry, &blockItems, "tempest_experiment");
    RegisterBlockItem(registry, &blockItems, "prism_experiment");
    RegisterBlockItem(registry, &blockItems, "hellgate_experiment");
    RegisterBlockItem(registry, &blockItems, "icicle", "assets/texture/item/icicle.png");
    RegisterBlockItem(registry, &blockItems, "ice_sheet", "assets/texture/block/ice_sheet.png");
    RegisterBlockItem(registry, &blockItems, "magma");
    RegisterBlockItem(registry, &blockItems, "brimstone");
    RegisterBlockItem(registry, &blockItems, "lily_pad", "assets/texture/block/lily_pad.png");
    RegisterBlockItem(registry, &blockItems, "scaffolding");

    static FlintAndSteelItem flintAndSteel;
    flintAndSteel.SetIconPath("assets/texture/item/flint_and_steel.png");
    flintAndSteel.SetMaxStackSize(1);
    flintAndSteel.SetMaxDamage(64);
    flintAndSteel.SetId(registry->Register("flint_and_steel", &flintAndSteel));

    static SpawnEggItem pigSpawnEgg(Pig::s_type, 0xF0A5A2, 0xDB635F);
    pigSpawnEgg.SetIconPath("spawn_egg/pig");
    pigSpawnEgg.SetMaxStackSize(16);
    pigSpawnEgg.SetId(registry->Register("pig_spawn_egg", &pigSpawnEgg));

    static SpawnEggItem cowSpawnEgg(Cow::s_type, 0x443626, 0xA1A1A1);
    cowSpawnEgg.SetIconPath("spawn_egg/cow");
    cowSpawnEgg.SetMaxStackSize(16);
    cowSpawnEgg.SetId(registry->Register("cow_spawn_egg", &cowSpawnEgg));

    static SpawnEggItem sheepSpawnEgg(Sheep::s_type, 0xE7E7E7, 0xFFB5B5);
    sheepSpawnEgg.SetIconPath("spawn_egg/sheep");
    sheepSpawnEgg.SetMaxStackSize(16);
    sheepSpawnEgg.SetId(registry->Register("sheep_spawn_egg", &sheepSpawnEgg));

    static SpawnEggItem chickenSpawnEgg(Chicken::s_type, 0xA1A1A1, 0xFF0000);
    chickenSpawnEgg.SetIconPath("spawn_egg/chicken");
    chickenSpawnEgg.SetMaxStackSize(16);
    chickenSpawnEgg.SetId(registry->Register("chicken_spawn_egg", &chickenSpawnEgg));

    static SpawnEggItem parrotSpawnEgg(Parrot::s_type, 0x0DA70B, 0xFF0000);
    parrotSpawnEgg.SetIconPath("spawn_egg/parrot");
    parrotSpawnEgg.SetMaxStackSize(16);
    parrotSpawnEgg.SetId(registry->Register("parrot_spawn_egg", &parrotSpawnEgg));

    static SpawnEggItem ocelotSpawnEgg(Ocelot::s_type, 0xEFDE7D, 0x564434);
    ocelotSpawnEgg.SetIconPath("spawn_egg/ocelot");
    ocelotSpawnEgg.SetMaxStackSize(16);
    ocelotSpawnEgg.SetId(registry->Register("ocelot_spawn_egg", &ocelotSpawnEgg));

    static SpawnEggItem squidSpawnEgg(Squid::s_type, 0x223B4D, 0x708899);
    squidSpawnEgg.SetIconPath("spawn_egg/squid");
    squidSpawnEgg.SetMaxStackSize(16);
    squidSpawnEgg.SetId(registry->Register("squid_spawn_egg", &squidSpawnEgg));

    static SpawnEggItem zombieSpawnEgg(Zombie::s_type, 0x00AFAF, 0x799C65);
    zombieSpawnEgg.SetIconPath("spawn_egg/zombie");
    zombieSpawnEgg.SetMaxStackSize(16);
    zombieSpawnEgg.SetId(registry->Register("zombie_spawn_egg", &zombieSpawnEgg));

    static SpawnEggItem skeletonSpawnEgg(Skeleton::s_type, 0xC1C1C1, 0x494949);
    skeletonSpawnEgg.SetIconPath("spawn_egg/skeleton");
    skeletonSpawnEgg.SetMaxStackSize(16);
    skeletonSpawnEgg.SetId(registry->Register("skeleton_spawn_egg", &skeletonSpawnEgg));

    static SpawnEggItem spiderSpawnEgg(Spider::s_type, 0x342D27, 0xA80E0E);
    spiderSpawnEgg.SetIconPath("spawn_egg/spider");
    spiderSpawnEgg.SetMaxStackSize(16);
    spiderSpawnEgg.SetId(registry->Register("spider_spawn_egg", &spiderSpawnEgg));

    static SpawnEggItem creeperSpawnEgg(Creeper::s_type, 0x0DA70B, 0x000000);
    creeperSpawnEgg.SetIconPath("spawn_egg/creeper");
    creeperSpawnEgg.SetMaxStackSize(16);
    creeperSpawnEgg.SetId(registry->Register("creeper_spawn_egg", &creeperSpawnEgg));

    static ShearsItem shears;
    shears.SetIconPath("assets/texture/item/shears.png");
    shears.SetMaxStackSize(1);
    shears.SetMaxDamage(238);
    shears.SetId(registry->Register("shears", &shears));

    static BucketItem bucket(nullptr);
    bucket.SetIconPath("assets/texture/item/bucket_empty.png");
    bucket.SetMaxStackSize(16);
    bucket.SetId(registry->Register("bucket", &bucket));

    static BucketItem waterBucket(Block::ByName("water_flow"));
    waterBucket.SetIconPath("assets/texture/item/bucket_water.png");
    waterBucket.SetMaxStackSize(1);
    waterBucket.SetId(registry->Register("water_bucket", &waterBucket));

    static MilkBucketItem milkBucket;
    milkBucket.SetIconPath("assets/texture/item/bucket_milk.png");
    milkBucket.SetMaxStackSize(1);
    milkBucket.SetId(registry->Register("milk_bucket", &milkBucket));

    static BucketItem lavaBucket(Block::ByName("lava_flow"));
    lavaBucket.SetIconPath("assets/texture/item/bucket_lava.png");
    lavaBucket.SetMaxStackSize(1);
    lavaBucket.SetId(registry->Register("lava_bucket", &lavaBucket));

    static BoatItem boats[Boat::kWoodTypeCount] = {BoatItem(0), BoatItem(1), BoatItem(2), BoatItem(3), BoatItem(4), BoatItem(5), BoatItem(6)};
    for (int i = 0; i < Boat::kWoodTypeCount; i++)
    {
        std::string name = std::string(Boat::kWoodTypes[i]) + "_boat";
        boats[i].SetIconPath("assets/texture/item/" + name + ".png");
        boats[i].SetMaxStackSize(1);
        boats[i].SetId(registry->Register(name, &boats[i]));
    }

    static Item clayBall;
    clayBall.SetIconPath("assets/texture/item/clay_ball.png");
    clayBall.SetId(registry->Register("clay_ball", &clayBall));

    static SnowballItem snowball;
    snowball.SetIconPath("assets/texture/item/snowball.png");
    snowball.SetMaxStackSize(16);
    snowball.SetId(registry->Register("snowball", &snowball));

    static EggItem egg;
    egg.SetIconPath("assets/texture/item/egg.png");
    egg.SetMaxStackSize(16);
    egg.SetId(registry->Register("egg", &egg));

    static Item feather;
    feather.SetIconPath("assets/texture/item/feather.png");
    feather.SetId(registry->Register("feather", &feather));

    static BowItem bow;
    bow.SetIconPath("assets/texture/item/bow.png");
    bow.SetMaxStackSize(1);
    bow.SetMaxDamage(384);
    bow.SetHandEquipped(true);
    bow.SetId(registry->Register("bow", &bow));

    static Item arrow;
    arrow.SetIconPath("assets/texture/item/arrow.png");
    arrow.SetId(registry->Register("arrow", &arrow));

    static FoodItem apple(4, 0.3f);
    apple.SetIconPath("assets/texture/item/apple.png");
    apple.SetId(registry->Register("apple", &apple));

    static FoodItem bread(5, 0.6f);
    bread.SetIconPath("assets/texture/item/bread.png");
    bread.SetId(registry->Register("bread", &bread));

    static FoodItem porkchop(3, 0.3f);
    porkchop.SetIconPath("assets/texture/item/porkchop.png");
    porkchop.SetId(registry->Register("porkchop", &porkchop));

    static FoodItem cookedPorkchop(8, 0.8f);
    cookedPorkchop.SetIconPath("assets/texture/item/cooked_porkchop.png");
    cookedPorkchop.SetId(registry->Register("cooked_porkchop", &cookedPorkchop));

    static FoodItem beef(3, 0.3f);
    beef.SetIconPath("assets/texture/item/beef.png");
    beef.SetId(registry->Register("beef", &beef));

    static FoodItem cookedBeef(8, 0.8f);
    cookedBeef.SetIconPath("assets/texture/item/cooked_beef.png");
    cookedBeef.SetId(registry->Register("cooked_beef", &cookedBeef));

    static FoodItem chicken(2, 0.3f);
    chicken.SetIconPath("assets/texture/item/chicken.png");
    chicken.SetHunger(0.3f, 600);
    chicken.SetId(registry->Register("chicken", &chicken));

    static FoodItem cookedChicken(6, 0.6f);
    cookedChicken.SetIconPath("assets/texture/item/cooked_chicken.png");
    cookedChicken.SetId(registry->Register("cooked_chicken", &cookedChicken));

    static FoodItem rottenFlesh(4, 0.1f);
    rottenFlesh.SetIconPath("assets/texture/item/rotten_flesh.png");
    rottenFlesh.SetHunger(0.8f, 600);
    rottenFlesh.SetId(registry->Register("rotten_flesh", &rottenFlesh));

    static FoodItem spiderEye(2, 0.8f);
    spiderEye.SetIconPath("assets/texture/item/spider_eye.png");
    spiderEye.SetPoison(1.0f, 100);
    spiderEye.SetId(registry->Register("spider_eye", &spiderEye));

    static SeedItem wheatSeeds(Block::ByName("wheat"));
    wheatSeeds.SetIconPath("assets/texture/item/wheat_seeds.png");
    wheatSeeds.SetId(registry->Register("wheat_seeds", &wheatSeeds));

    static Item wheat;
    wheat.SetIconPath("assets/texture/item/wheat.png");
    wheat.SetId(registry->Register("wheat", &wheat));

    static FoodItem carrot(3, 0.6f, Block::ByName("carrots"));
    carrot.SetIconPath("assets/texture/item/carrot.png");
    carrot.SetId(registry->Register("carrot", &carrot));

    static FoodItem goldenCarrot(6, 1.2f);
    goldenCarrot.SetIconPath("assets/texture/item/golden_carrot.png");
    goldenCarrot.SetId(registry->Register("golden_carrot", &goldenCarrot));

    static FoodItem potato(1, 0.3f, Block::ByName("potatoes"));
    potato.SetIconPath("assets/texture/item/potato.png");
    potato.SetId(registry->Register("potato", &potato));

    static FoodItem bakedPotato(5, 0.6f);
    bakedPotato.SetIconPath("assets/texture/item/baked_potato.png");
    bakedPotato.SetId(registry->Register("baked_potato", &bakedPotato));

    static FoodItem poisonousPotato(2, 0.3f);
    poisonousPotato.SetIconPath("assets/texture/item/poisonous_potato.png");
    poisonousPotato.SetPoison(0.6f, 100);
    poisonousPotato.SetId(registry->Register("poisonous_potato", &poisonousPotato));

    static SeedItem beetrootSeeds(Block::ByName("beetroots"));
    beetrootSeeds.SetIconPath("assets/texture/item/beetroot_seeds.png");
    beetrootSeeds.SetId(registry->Register("beetroot_seeds", &beetrootSeeds));

    static FoodItem beetroot(1, 0.6f);
    beetroot.SetIconPath("assets/texture/item/beetroot.png");
    beetroot.SetId(registry->Register("beetroot", &beetroot));

    static Item bowl;
    bowl.SetIconPath("assets/texture/item/bowl.png");
    bowl.SetId(registry->Register("bowl", &bowl));

    static FoodItem beetrootSoup(6, 0.6f);
    beetrootSoup.SetRemainder(&bowl);
    beetrootSoup.SetIconPath("assets/texture/item/beetroot_soup.png");
    beetrootSoup.SetMaxStackSize(1);
    beetrootSoup.SetId(registry->Register("beetroot_soup", &beetrootSoup));

    static SeedItem pumpkinSeeds(Block::ByName("pumpkin_stem"));
    pumpkinSeeds.SetIconPath("assets/texture/item/pumpkin_seeds.png");
    pumpkinSeeds.SetId(registry->Register("pumpkin_seeds", &pumpkinSeeds));

    static SeedItem melonSeeds(Block::ByName("melon_stem"));
    melonSeeds.SetIconPath("assets/texture/item/melon_seeds.png");
    melonSeeds.SetId(registry->Register("melon_seeds", &melonSeeds));

    static SeedItem cocoaBeans(Block::ByName("cocoa"));
    cocoaBeans.SetIconPath("assets/texture/item/cocoa_beans.png");
    cocoaBeans.SetId(registry->Register("cocoa_beans", &cocoaBeans));

    static SeedItem bamboo(Block::ByName("bamboo_sapling"));
    bamboo.SetIconPath("assets/texture/item/bamboo.png");
    bamboo.SetId(registry->Register("bamboo", &bamboo));

    static FoodItem sweetBerries(2, 0.1f, Block::ByName("sweet_berry_bush"));
    sweetBerries.SetIconPath("assets/texture/item/sweet_berries.png");
    sweetBerries.SetId(registry->Register("sweet_berries", &sweetBerries));

    static FoodItem melonSlice(2, 0.3f);
    melonSlice.SetIconPath("assets/texture/item/melon_slice.png");
    melonSlice.SetId(registry->Register("melon_slice", &melonSlice));

    static FoodItem pumpkinPie(8, 0.3f);
    pumpkinPie.SetIconPath("assets/texture/item/pumpkin_pie.png");
    pumpkinPie.SetId(registry->Register("pumpkin_pie", &pumpkinPie));

    constexpr const char *kToolMaterials[]                = {"wooden", "stone", "iron", "golden", "diamond", "severite"};
    constexpr int kToolLevels[]                           = {0, 1, 2, 0, 3, 3};
    constexpr int kToolUses[]                             = {59, 131, 250, 32, 1561, 2031};
    constexpr float kToolSpeeds[]                         = {2.0f, 4.0f, 6.0f, 12.0f, 8.0f, 9.0f};
    constexpr int kToolAttackBonus[]                      = {0, 1, 2, 0, 3, 4};
    const std::vector<ResourceLocation> toolRepairItems[] = {
            {"oak_planks", "spruce_planks", "birch_planks", "acacia_planks", "jungle_planks", "cherry_planks", "dark_oak_planks", "bamboo_planks"},
            {"cobblestone"},
            {"iron_ingot"},
            {"gold_ingot"},
            {"diamond"},
            {"severite"}};
    constexpr const char *kDiggerTypes[]           = {"shovel", "pickaxe", "axe"};
    constexpr int kDiggerAttackDamage[]            = {1, 2, 3};
    constexpr Block::eTOOL_TYPE kDiggerToolTypes[] = {Block::eToolType_Shovel, Block::eToolType_Pickaxe, Block::eToolType_Axe};
    static SwordItem swords[std::size(kToolMaterials)];
    static ShovelItem shovels[std::size(kToolMaterials)];
    static DiggerItem diggers[std::size(kToolMaterials) * (std::size(kDiggerTypes) - 1)];
    static HoeItem hoes[std::size(kToolMaterials)];
    for (size_t i = 0; i < std::size(kToolMaterials); i++)
    {
        for (size_t j = 0; j <= std::size(kDiggerTypes) + 1; j++)
        {
            Item *tool = nullptr;
            std::string type;
            if (j == 0)
            {
                tool = &swords[i];
                tool->SetAttackDamage(4 + kToolAttackBonus[i]);
                type = "sword";
            }
            else if (j <= std::size(kDiggerTypes))
            {
                DiggerItem *digger = j == 1 ? &shovels[i] : &diggers[i * (std::size(kDiggerTypes) - 1) + j - 2];
                digger->SetTier(kDiggerToolTypes[j - 1], kToolLevels[i], kToolSpeeds[i]);
                tool = digger;
                tool->SetAttackDamage(kDiggerAttackDamage[j - 1] + kToolAttackBonus[i]);
                type = kDiggerTypes[j - 1];
            }
            else
            {
                tool = &hoes[i];
                type = "hoe";
            }

            std::string name = std::string(kToolMaterials[i]) + "_" + type;
            tool->SetIconPath("assets/texture/item/" + name + ".png");
            tool->SetHandEquipped(true);
            tool->SetMaxStackSize(1);
            tool->SetMaxDamage(kToolUses[i]);
            tool->SetRepairItems(toolRepairItems[i]);
            tool->SetId(registry->Register(name, tool));
        }
    }

    constexpr const char *kArmorMaterials[]   = {"leather", "chainmail", "iron", "golden", "diamond", "severite"};
    constexpr const char *kArmorLayers[]      = {"leather", "chainmail", "iron", "gold", "diamond", "severite"};
    constexpr int kArmorDurability[]          = {5, 15, 15, 7, 33, 40};
    constexpr const char *kArmorRepairItems[] = {"leather", "iron_ingot", "iron_ingot", "gold_ingot", "diamond", "severite"};
    constexpr int kArmorDefense[][4]          = {{1, 3, 2, 1}, {2, 5, 4, 1}, {2, 6, 5, 2}, {2, 5, 3, 1}, {3, 8, 6, 3}, {3, 9, 7, 3}};
    constexpr const char *kArmorPieces[]      = {"helmet", "chestplate", "leggings", "boots"};
    constexpr int kArmorHealthPerSlot[]       = {11, 16, 15, 13};
    static ArmorItem armor[std::size(kArmorMaterials) * std::size(kArmorPieces)];
    for (size_t i = 0; i < std::size(kArmorMaterials); i++)
    {
        for (int j = 0; j < (int) std::size(kArmorPieces); j++)
        {
            ArmorItem *piece  = &armor[i * std::size(kArmorPieces) + (size_t) j];
            std::string name  = std::string(kArmorMaterials[i]) + "_" + kArmorPieces[j];
            std::string layer = std::string(kArmorLayers[i]) + (j == ArmorItem::s_legs ? "_layer_2.png" : "_layer_1.png");
            piece->SetArmor(j, kArmorDefense[i][j], "assets/texture/models/armor/" + layer);
            piece->SetIconPath("assets/texture/item/" + name + ".png");
            piece->SetMaxStackSize(1);
            piece->SetMaxDamage(kArmorHealthPerSlot[j] * kArmorDurability[i]);
            piece->SetRepairItems({kArmorRepairItems[i]});
            piece->SetId(registry->Register(name, piece));
        }
    }

    const char *materialNames[] = {"coal",           "diamond",         "emerald",        "lapis_lazuli",   "iron_ingot",
                                   "iron_nugget",    "charcoal",        "gold_ingot",     "leather",        "string",
                                   "flint",          "gunpowder",       "bone",           "light_gray_dye", "gray_dye",
                                   "black_dye",      "red_dye",         "orange_dye",     "yellow_dye",     "lime_dye",
                                   "green_dye",      "cyan_dye",        "light_blue_dye", "blue_dye",       "purple_dye",
                                   "magenta_dye",    "pink_dye",        "glass_shard",    "ice_shard",      "lead",
                                   "resonant_shard", "rift_ember",      "severite",       "gold_nugget",    "diamond_nugget",
                                   "emerald_nugget", "severite_nugget", "quartz"};
    static Item materials[std::size(materialNames)];
    for (size_t i = 0; i < std::size(materialNames); i++)
    {
        materials[i].SetIconPath(std::string("assets/texture/item/") + materialNames[i] + ".png");
        materials[i].SetId(registry->Register(materialNames[i], &materials[i]));
    }

    const char *oreNames[] = {"coal_ore",   "iron_ore",   "gold_ore",   "diamond_ore",   "emerald_ore",   "lapis_ore",
                              "coal_block", "iron_block", "gold_block", "diamond_block", "emerald_block", "lapis_block"};
    for (size_t i = 0; i < std::size(oreNames); i++)
    {
        RegisterBlockItem(registry, &blockItems, oreNames[i]);
    }

    RegisterBlockItem(registry, &blockItems, "fire");
    RegisterBlockItem(registry, &blockItems, "farmland");
    RegisterBlockItem(registry, &blockItems, "dirt_path");
    RegisterBlockItem(registry, &blockItems, "lit_furnace");
    RegisterBlockItem(registry, &blockItems, "carrots");
    RegisterBlockItem(registry, &blockItems, "potatoes");
    RegisterBlockItem(registry, &blockItems, "beetroots");
    RegisterBlockItem(registry, &blockItems, "pumpkin_stem");
    RegisterBlockItem(registry, &blockItems, "attached_pumpkin_stem");
    RegisterBlockItem(registry, &blockItems, "melon_stem");
    RegisterBlockItem(registry, &blockItems, "attached_melon_stem");
    RegisterBlockItem(registry, &blockItems, "oak_trapdoor");
    RegisterBlockItem(registry, &blockItems, "spruce_trapdoor");
    RegisterBlockItem(registry, &blockItems, "birch_trapdoor");
    RegisterBlockItem(registry, &blockItems, "acacia_trapdoor");
    RegisterBlockItem(registry, &blockItems, "jungle_trapdoor");
    RegisterBlockItem(registry, &blockItems, "cherry_trapdoor");
    RegisterBlockItem(registry, &blockItems, "bamboo_trapdoor");
    RegisterBlockItem(registry, &blockItems, "cobblestone_wall");
    RegisterBlockItem(registry, &blockItems, "mossy_cobblestone_wall");
    const char *variantOreNames[] = {"sandstone_coal_ore",    "sandstone_iron_ore",  "sandstone_gold_ore", "sandstone_diamond_ore",
                                     "sandstone_emerald_ore", "sandstone_lapis_ore", "ice_coal_ore",       "ice_iron_ore",
                                     "ice_gold_ore",          "ice_diamond_ore",     "ice_emerald_ore",    "ice_lapis_ore",
                                     "basalt_coal_ore",       "basalt_iron_ore",     "basalt_gold_ore",    "basalt_diamond_ore",
                                     "basalt_emerald_ore",    "basalt_lapis_ore",    "basalt_quartz_ore"};
    for (size_t i = 0; i < std::size(variantOreNames); i++)
    {
        RegisterBlockItem(registry, &blockItems, variantOreNames[i]);
    }

    RegisterBlockItem(registry, &blockItems, "brittle_ice");
    RegisterBlockItem(registry, &blockItems, "dark_oak_log");
    RegisterBlockItem(registry, &blockItems, "dark_oak_planks");
    RegisterBlockItem(registry, &blockItems, "dark_oak_leaves");
    RegisterBlockItem(registry, &blockItems, "dark_oak_sapling");
    RegisterBlockItem(registry, &blockItems, "dark_oak_door", "assets/texture/item/door_dark_oak.png");
    RegisterBlockItem(registry, &blockItems, "dark_oak_fence");
    RegisterBlockItem(registry, &blockItems, "dark_oak_fence_gate");
    RegisterBlockItem(registry, &blockItems, "dark_oak_button");
    RegisterBlockItem(registry, &blockItems, "dark_oak_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "dark_oak_slab");
    RegisterBlockItem(registry, &blockItems, "dark_oak_stairs");
    RegisterBlockItem(registry, &blockItems, "dark_oak_trapdoor");
    RegisterBlockItem(registry, &blockItems, "basalt");

    const char *terracottaColors[] = {"white", "orange", "yellow", "light_gray", "brown",  "red",  "magenta", "light_blue",
                                      "lime",  "pink",   "gray",   "cyan",       "purple", "blue", "green",   "black"};
    for (size_t i = 0; i < std::size(terracottaColors); i++)
    {
        RegisterBlockItem(registry, &blockItems, std::string(terracottaColors[i]) + "_terracotta");
    }

    const char *signWoods[] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak", "cherry", "bamboo"};
    for (size_t i = 0; i < std::size(signWoods); i++)
    {
        std::string name = std::string(signWoods[i]) + "_sign";
        RegisterBlockItem(registry, &blockItems, name, "assets/texture/item/" + name + ".png");
        Item::ByName(name)->SetMaxStackSize(16);
    }

    static Item inkSac;
    inkSac.SetIconPath("assets/texture/item/ink_sac.png");
    inkSac.SetId(registry->Register("ink_sac", &inkSac));

    static Item glowInkSac;
    glowInkSac.SetIconPath("assets/texture/item/glow_ink_sac.png");
    glowInkSac.SetId(registry->Register("glow_ink_sac", &glowInkSac));

    static Item honeycomb;
    honeycomb.SetIconPath("assets/texture/item/honeycomb.png");
    honeycomb.SetId(registry->Register("honeycomb", &honeycomb));

    RegisterBlockItem(registry, &blockItems, "bookshelf");

    static Item paper;
    paper.SetIconPath("assets/texture/item/paper.png");
    paper.SetId(registry->Register("paper", &paper));

    static Item book;
    book.SetIconPath("assets/texture/item/book.png");
    book.SetId(registry->Register("book", &book));

    static Item fracturedBook;
    fracturedBook.SetIconPath("assets/texture/item/fractured_book.png");
    fracturedBook.SetMaxStackSize(1);
    fracturedBook.SetId(registry->Register("fractured_book", &fracturedBook));

    static SeedItem redstone(Block::ByName("redstone_wire"));
    redstone.SetIconPath("assets/texture/item/redstone.png");
    redstone.SetId(registry->Register("redstone", &redstone));

    const char *redstoneBlocks[] = {"redstone_ore",     "redstone_block",      "sandstone_redstone_ore",
                                    "ice_redstone_ore", "basalt_redstone_ore", "redstone_torch"};
    for (size_t i = 0; i < std::size(redstoneBlocks); i++)
    {
        RegisterBlockItem(registry, &blockItems, redstoneBlocks[i]);
    }

    RegisterBlockItem(registry, &blockItems, "hopper", "assets/texture/item/hopper.png");
    RegisterBlockItem(registry, &blockItems, "redstone_lamp");
    RegisterBlockItem(registry, &blockItems, "note_block");
    RegisterBlockItem(registry, &blockItems, "target");
    RegisterBlockItem(registry, &blockItems, "light_weighted_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "heavy_weighted_pressure_plate");
    RegisterBlockItem(registry, &blockItems, "repeater", "assets/texture/item/repeater.png");
    RegisterBlockItem(registry, &blockItems, "comparator", "assets/texture/item/comparator.png");
    RegisterBlockItem(registry, &blockItems, "piston");
    RegisterBlockItem(registry, &blockItems, "observer");
    RegisterBlockItem(registry, &blockItems, "dispenser");
    RegisterBlockItem(registry, &blockItems, "dropper");
    RegisterBlockItem(registry, &blockItems, "slime_block");
    RegisterBlockItem(registry, &blockItems, "sticky_piston");
    RegisterBlockItem(registry, &blockItems, "rail", "assets/texture/block/rail_normal.png");
    RegisterBlockItem(registry, &blockItems, "powered_rail", "assets/texture/block/rail_golden.png");
    RegisterBlockItem(registry, &blockItems, "detector_rail", "assets/texture/block/rail_detector.png");

    static MinecartItem minecart(Minecart::eMinecartType_Rideable);
    minecart.SetIconPath("assets/texture/item/minecart.png");
    minecart.SetMaxStackSize(1);
    minecart.SetId(registry->Register("minecart", &minecart));

    static MinecartItem chestMinecart(Minecart::eMinecartType_Chest);
    chestMinecart.SetIconPath("assets/texture/item/chest_minecart.png");
    chestMinecart.SetMaxStackSize(1);
    chestMinecart.SetId(registry->Register("chest_minecart", &chestMinecart));

    static MinecartItem furnaceMinecart(Minecart::eMinecartType_Furnace);
    furnaceMinecart.SetIconPath("assets/texture/item/furnace_minecart.png");
    furnaceMinecart.SetMaxStackSize(1);
    furnaceMinecart.SetId(registry->Register("furnace_minecart", &furnaceMinecart));

    static MinecartItem tntMinecart(Minecart::eMinecartType_Tnt);
    tntMinecart.SetIconPath("assets/texture/item/tnt_minecart.png");
    tntMinecart.SetMaxStackSize(1);
    tntMinecart.SetId(registry->Register("tnt_minecart", &tntMinecart));

    static MinecartItem hopperMinecart(Minecart::eMinecartType_Hopper);
    hopperMinecart.SetIconPath("assets/texture/item/hopper_minecart.png");
    hopperMinecart.SetMaxStackSize(1);
    hopperMinecart.SetId(registry->Register("hopper_minecart", &hopperMinecart));

    static Item slimeBall;
    slimeBall.SetIconPath("assets/texture/item/slime_ball.png");
    slimeBall.SetId(registry->Register("slime_ball", &slimeBall));

    static SpawnEggItem slimeSpawnEgg(Slime::s_type, 0x51A03E, 0x7EBF6E);
    slimeSpawnEgg.SetIconPath("spawn_egg/slime");
    slimeSpawnEgg.SetMaxStackSize(16);
    slimeSpawnEgg.SetId(registry->Register("slime_spawn_egg", &slimeSpawnEgg));

    std::unordered_set<Block *> placedBlocks;
    for (uint32_t id = 0; id < registry->Size(); id++)
    {
        placedBlocks.insert(registry->ById(id)->GetBlock());
    }

    // These are states or parts of another block that its own item already places
    const std::unordered_set<std::string> stateBlocks = {
            "air", "moving_piston", "piston_head", "unlit_redstone_torch", "unlit_redstone_torch_wall", "lit_redstone_lamp"};
    BlockRegistry *blocks = BlockRegistry::GetInstance();
    for (uint32_t id = 0; id < blocks->Size(); id++)
    {
        const ResourceLocation &name = blocks->GetKey(id);
        if (!placedBlocks.contains(blocks->ById(id)) && !registry->Has(name) && !stateBlocks.contains(name.GetPath()))
        {
            RegisterBlockItem(registry, &blockItems, name);
        }
    }
}
