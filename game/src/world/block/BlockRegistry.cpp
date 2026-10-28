#include "BlockRegistry.h"

#include <memory>
#include <vector>

#include "../../utils/Direction.h"
#include "AnvilBlock.h"
#include "AttachedStemBlock.h"
#include "BambooBlock.h"
#include "BambooSaplingBlock.h"
#include "BedBlock.h"
#include "BrittleIceBlock.h"
#include "ButtonBlock.h"
#include "CactusBlock.h"
#include "CarpetBlock.h"
#include "CataclysmExperimentBlock.h"
#include "ChainBlock.h"
#include "ChestBlock.h"
#include "ClimbableBlock.h"
#include "CocoaBlock.h"
#include "ComparatorBlock.h"
#include "CraftingTableBlock.h"
#include "CropBlock.h"
#include "DeadBushBlock.h"
#include "DetectorRailBlock.h"
#include "DirtPathBlock.h"
#include "DispenserBlock.h"
#include "DoorBlock.h"
#include "DoublePlantBlock.h"
#include "DropperBlock.h"
#include "EchoGlassBlock.h"
#include "EffectsExperimentBlock.h"
#include "EnchantedIceBlock.h"
#include "FarmlandBlock.h"
#include "FenceBlock.h"
#include "FenceGateBlock.h"
#include "FireBlock.h"
#include "FractureCoreBlock.h"
#include "FurnaceBlock.h"
#include "GrassBlock.h"
#include "GravityBlock.h"
#include "HopperBlock.h"
#include "IceBlock.h"
#include "IceSheetBlock.h"
#include "LanternBlock.h"
#include "LayerBlock.h"
#include "LeavesBlock.h"
#include "LeverBlock.h"
#include "LilyPadBlock.h"
#include "LiquidBlock.h"
#include "MelonBlock.h"
#include "MobSpawnerBlock.h"
#include "MovingPistonBlock.h"
#include "NoteBlock.h"
#include "ObserverBlock.h"
#include "PaneBlock.h"
#include "PinkPetalsBlock.h"
#include "PistonBaseBlock.h"
#include "PistonHeadBlock.h"
#include "PointedDripstoneBlock.h"
#include "PoweredBlock.h"
#include "PoweredRailBlock.h"
#include "PressurePlateBlock.h"
#include "PricklyVineBlock.h"
#include "RailBlock.h"
#include "RedstoneLampBlock.h"
#include "RedstoneTorchBlock.h"
#include "RedstoneWireBlock.h"
#include "RepeaterBlock.h"
#include "ResonantSocketBlock.h"
#include "ResonatorBlock.h"
#include "SaplingBlock.h"
#include "ScaffoldingBlock.h"
#include "SignBlock.h"
#include "SlabBlock.h"
#include "SlimeBlock.h"
#include "SnowLayerBlock.h"
#include "StairBlock.h"
#include "StemBlock.h"
#include "SugarCaneBlock.h"
#include "SunbloomBlock.h"
#include "SweetBerryBushBlock.h"
#include "TargetBlock.h"
#include "TntBlock.h"
#include "TrapDoorBlock.h"
#include "VineBlock.h"
#include "WallBlock.h"
#include "WeightedPressurePlateBlock.h"

BlockRegistry *BlockRegistry::GetInstance()
{
    static BlockRegistry instance;
    return &instance;
}

uint32_t BlockRegistry::Register(const ResourceLocation &key, Block *block)
{
    uint32_t id = MappedRegistry::Register(key, block);
    block->SetId(id);
    return id;
}

uint32_t BlockRegistry::IdOf(Block *block) const
{
    if (block == nullptr)
    {
        return 0;
    }

    return block->GetId();
}

void BlockRegistry::Init()
{
    BlockRegistry *registry = GetInstance();

    static Block air(false, "");
    air.SetBreakSound("");
    air.SetSelectable(false);

    static Block bedrock(true, "assets/texture/block/bedrock.png");
    static Block stone(true, "assets/texture/block/stone.png");
    static Block cobblestone(true, "assets/texture/block/cobblestone.png");
    static Block andesite(true, "assets/texture/block/andesite.png");
    static Block granite(true, "assets/texture/block/granite.png");
    static Block polishedGranite(true, "assets/texture/block/polished_granite.png");
    static Block diorite(true, "assets/texture/block/diorite.png");
    static Block polishedDiorite(true, "assets/texture/block/polished_diorite.png");
    static Block polishedAndesite(true, "assets/texture/block/polished_andesite.png");
    static Block stoneBricks(true, "assets/texture/block/stonebricks.png");
    static Block mossyStoneBricks(true, "assets/texture/block/mossy_stonebricks.png");
    static Block crackedStoneBricks(true, "assets/texture/block/cracked_stonebricks.png");
    static Block chiseledStoneBricks(true, "assets/texture/block/carved_stonebricks.png");
    static SlabBlock stoneSlab("assets/texture/block/stone.png", &stone);
    static StairBlock stoneStairs("assets/texture/block/stone.png");
    static SlabBlock stoneBrickSlab("assets/texture/block/stonebricks.png", &stoneBricks);
    static StairBlock stoneBrickStairs("assets/texture/block/stonebricks.png");

    static Block dirt(true, "assets/texture/block/dirt.png");
    dirt.SetBreakSound("dig.dirt");

    static GrassBlock grass_block("assets/texture/block/grass_side.png");
    grass_block.SetBreakSound("dig.grass");
    grass_block.SetTexturePath(Direction::up, "assets/texture/block/grass_top.png");
    grass_block.SetTexturePath(Direction::down, "assets/texture/block/dirt.png");
    grass_block.SetBiomeTint(Direction::up);
    grass_block.SetSideOverlay("assets/texture/block/grass_side_overlay.png");

    static Block grass(false, "assets/texture/block/grass.png");
    grass.SetBreakSound("dig.grass");
    grass.SetCollidable(false);
    grass.SetRenderShape(Block::eRenderShape_CROSS);
    grass.SetBiomeTint();
    grass.SetAABB(AABB(glm::dvec3(0.1, 0.0, 0.1), glm::dvec3(0.9, 0.8, 0.9)));
    grass.SetReplaceable(true);
    grass.SetNeedsSupportBelow(true);
    grass.SetSupportBlocks({"grass_block", "dirt", "coarse_dirt", "podzol"});

    static Block fern(false, "assets/texture/block/fern.png");
    fern.SetBreakSound("dig.grass");
    fern.SetCollidable(false);
    fern.SetRenderShape(Block::eRenderShape_CROSS);
    fern.SetBiomeTint();
    fern.SetAABB(AABB(glm::dvec3(0.1, 0.0, 0.1), glm::dvec3(0.9, 0.8, 0.9)));
    fern.SetReplaceable(true);
    fern.SetNeedsSupportBelow(true);
    fern.SetSupportBlocks({"grass_block", "dirt", "coarse_dirt", "podzol"});

    static GravityBlock sand("assets/texture/block/sand.png");
    sand.SetBreakSound("dig.sand");
    static LayerBlock sandLayer("assets/texture/block/sand.png", &sand);
    sandLayer.SetBreakSound("dig.sand");

    static GravityBlock gravel("assets/texture/block/gravel.png");
    gravel.SetBreakSound("dig.dirt");

    static Block clay(true, "assets/texture/block/clay.png");
    clay.SetBreakSound("dig.dirt");

    static Block podzol(true, "assets/texture/block/podzol_side.png");
    podzol.SetBreakSound("dig.dirt");
    podzol.SetTexturePath(Direction::up, "assets/texture/block/podzol_top.png");
    podzol.SetTexturePath(Direction::down, "assets/texture/block/dirt.png");

    static Block coarseDirt(true, "assets/texture/block/coarse_dirt.png");
    coarseDirt.SetBreakSound("dig.dirt");

    static FurnaceBlock furnace("assets/texture/block/furnace_side.png", "assets/texture/block/furnace_top.png",
                                "assets/texture/block/furnace_front_off.png", true, false);
    static FurnaceBlock litFurnace("assets/texture/block/furnace_side.png", "assets/texture/block/furnace_top.png",
                                   "assets/texture/block/furnace_front_on.png", true, true);
    litFurnace.SetLightEmission(13);
    litFurnace.SetLightColor(0xFF, 0xE0, 0xB0);

    static SnowBlock snowBlock("assets/texture/block/snow.png");
    snowBlock.SetBreakSound("dig.snow");

    static SnowLayerBlock snow("assets/texture/block/snow.png", &snowBlock);
    snow.SetBreakSound("dig.snow");

    static LiquidBlock waterStill("assets/texture/block/water_still.png", false, false);

    static LiquidBlock waterFlowBlock("assets/texture/block/water_flow.png", true, false);
    waterStill.SetCounterpart(&waterFlowBlock);
    waterStill.SetBreakSound("");
    waterFlowBlock.SetBreakSound("");
    waterFlowBlock.SetCounterpart(&waterStill);

    static LiquidBlock lavaStill("assets/texture/block/lava_still.png", false, true);

    static LiquidBlock lavaFlowBlock("assets/texture/block/lava_flow.png", true, true);
    lavaStill.SetCounterpart(&lavaFlowBlock);
    lavaFlowBlock.SetCounterpart(&lavaStill);
    lavaStill.SetBreakSound("");
    lavaFlowBlock.SetBreakSound("");
    lavaStill.SetLightEmission(15);
    lavaFlowBlock.SetLightEmission(15);
    lavaStill.SetLightColor(0xFF, 0xA0, 0x50);
    lavaFlowBlock.SetLightColor(0xFF, 0xA0, 0x50);
    lavaStill.SetLightOpacity(15);
    lavaFlowBlock.SetLightOpacity(15);

    static Block torch(false, "assets/texture/block/torch.png");
    torch.SetBreakSound("dig.wood");
    torch.SetRenderShape(Block::eRenderShape_TORCH);
    torch.SetAABB(AABB(glm::dvec3(0.4375f, 0.0f, 0.4375f), glm::dvec3(0.5625f, 0.625f, 0.5625f)));
    torch.SetNeedsAttachedSupport(true);
    torch.SetNeedsCenterSupport(true);
    torch.SetLightEmission(14);
    torch.SetLightColor(0xFF, 0xE0, 0xB0);

    float sideU0 = 7.0f / 16.0f;
    float sideU1 = 9.0f / 16.0f;
    float sideV0 = 6.0f / 16.0f;
    float sideV1 = 16.0f / 16.0f;
    torch.SetUVRect(Direction::north, sideU0, sideV0, sideU1, sideV1);
    torch.SetUVRect(Direction::south, sideU0, sideV0, sideU1, sideV1);
    torch.SetUVRect(Direction::east, sideU0, sideV0, sideU1, sideV1);
    torch.SetUVRect(Direction::west, sideU0, sideV0, sideU1, sideV1);
    torch.SetUVRect(Direction::up, 7.0f / 16.0f, 6.0f / 16.0f, 9.0f / 16.0f, 8.0f / 16.0f);
    torch.SetUVRect(Direction::down, 7.0f / 16.0f, 14.0f / 16.0f, 9.0f / 16.0f, 16.0f / 16.0f);

    static Block torchWall(false, "assets/texture/block/torch.png");
    torchWall.SetBreakSound("dig.wood");
    torchWall.SetRenderShape(Block::eRenderShape_TORCH);
    torchWall.SetAABB(AABB(glm::dvec3(0.4375f, 0.2f, 0.4375f), glm::dvec3(0.5625f, 0.825f, 0.5625f)));
    torchWall.SetInteractionAABB(AABB(glm::dvec3(0.4f, 0.2f, 0.4f), glm::dvec3(0.6f, 0.8f, 0.6f)));
    torchWall.SetInteractionAttachmentOffset(-0.12f);
    torchWall.SetNeedsAttachedSupport(true);
    torchWall.SetLightEmission(14);
    torchWall.SetLightColor(0xFF, 0xE0, 0xB0);
    torchWall.SetUVRect(Direction::north, sideU0, sideV0, sideU1, sideV1);
    torchWall.SetUVRect(Direction::south, sideU0, sideV0, sideU1, sideV1);
    torchWall.SetUVRect(Direction::east, sideU0, sideV0, sideU1, sideV1);
    torchWall.SetUVRect(Direction::west, sideU0, sideV0, sideU1, sideV1);
    torchWall.SetUVRect(Direction::up, 7.0f / 16.0f, 6.0f / 16.0f, 9.0f / 16.0f, 8.0f / 16.0f);
    torchWall.SetUVRect(Direction::down, 7.0f / 16.0f, 14.0f / 16.0f, 9.0f / 16.0f, 16.0f / 16.0f);
    torchWall.SetWallMountedTransform(22.5f, 0.475f);
    torchWall.SetHorizontalFacing();

    torch.SetWallVariant(&torchWall);

    static Block oakLog(true, "assets/texture/block/log_oak.png");
    oakLog.SetBreakSound("dig.wood");
    oakLog.SetPillar("assets/texture/block/log_oak_top.png");

    static Block spruceLog(true, "assets/texture/block/log_spruce.png");
    spruceLog.SetBreakSound("dig.wood");
    spruceLog.SetPillar("assets/texture/block/log_spruce_top.png");

    static Block oakPlanks(true, "assets/texture/block/planks_oak.png");
    oakPlanks.SetBreakSound("dig.wood");

    static Block sprucePlanks(true, "assets/texture/block/planks_spruce.png");
    sprucePlanks.SetBreakSound("dig.wood");

    static SlabBlock oakSlab("assets/texture/block/planks_oak.png", &oakPlanks);
    oakSlab.SetBreakSound("dig.wood");

    static StairBlock oakStairs("assets/texture/block/planks_oak.png");
    oakStairs.SetBreakSound("dig.wood");

    static SlabBlock spruceSlab("assets/texture/block/planks_spruce.png", &sprucePlanks);
    spruceSlab.SetBreakSound("dig.wood");

    static StairBlock spruceStairs("assets/texture/block/planks_spruce.png");
    spruceStairs.SetBreakSound("dig.wood");

    static LeavesBlock oakLeaves("assets/texture/block/leaves_oak.png");
    oakLeaves.SetFoliageTint();

    static LeavesBlock spruceLeaves("assets/texture/block/leaves_spruce.png");
    spruceLeaves.SetFixedTint(0x619961);

    static Block birchLog(true, "assets/texture/block/birch_log.png");
    birchLog.SetBreakSound("dig.wood");
    birchLog.SetPillar("assets/texture/block/birch_log_top.png");

    static Block acaciaLog(true, "assets/texture/block/acacia_log.png");
    acaciaLog.SetBreakSound("dig.wood");
    acaciaLog.SetPillar("assets/texture/block/acacia_log_top.png");

    static Block birchPlanks(true, "assets/texture/block/birch_planks.png");
    birchPlanks.SetBreakSound("dig.wood");

    static Block acaciaPlanks(true, "assets/texture/block/acacia_planks.png");
    acaciaPlanks.SetBreakSound("dig.wood");

    static LeavesBlock birchLeaves("assets/texture/block/leaves_birch.png");
    birchLeaves.SetFixedTint(0x80A755);

    static LeavesBlock acaciaLeaves("assets/texture/block/leaves_acacia.png");
    acaciaLeaves.SetFoliageTint();

    static CraftingTableBlock craftingTable;
    craftingTable.SetBreakSound("dig.wood");
    craftingTable.SetFaceTexture(Direction::up, "assets/texture/block/crafting_table_top.png");
    craftingTable.SetFaceTexture(Direction::down, "assets/texture/block/planks_oak.png");
    craftingTable.SetFaceTexture(Direction::north, "assets/texture/block/crafting_table_front.png");
    craftingTable.SetFaceTexture(Direction::west, "assets/texture/block/crafting_table_front.png");

    static Block obsidian(true, "assets/texture/block/obsidian.png");
    static Block mossyCobblestone(true, "assets/texture/block/mossy_cobblestone.png");
    static Block checkeredTiles(true, "assets/texture/block/checkered_tiles.png");
    static Block whiteBricks(true, "assets/texture/block/white_bricks.png");
    static Block lightGrayBricks(true, "assets/texture/block/light_gray_bricks.png");
    static Block grayBricks(true, "assets/texture/block/gray_bricks.png");
    static Block blackBricks(true, "assets/texture/block/black_bricks.png");
    static Block brownBricks(true, "assets/texture/block/brown_bricks.png");
    static Block redBricks(true, "assets/texture/block/red_bricks.png");
    static Block vibrantRedBricks(true, "assets/texture/block/vibrant_red_bricks.png");
    static Block orangeBricks(true, "assets/texture/block/orange_bricks.png");
    static Block yellowBricks(true, "assets/texture/block/yellow_bricks.png");
    static Block limeBricks(true, "assets/texture/block/lime_bricks.png");
    static Block greenBricks(true, "assets/texture/block/green_bricks.png");
    static Block cyanBricks(true, "assets/texture/block/cyan_bricks.png");
    static Block lightBlueBricks(true, "assets/texture/block/light_blue_bricks.png");
    static Block blueBricks(true, "assets/texture/block/blue_bricks.png");
    static Block purpleBricks(true, "assets/texture/block/purple_bricks.png");
    static Block magentaBricks(true, "assets/texture/block/magenta_bricks.png");
    static Block pinkBricks(true, "assets/texture/block/pink_bricks.png");

    static Block whiteWool(true, "assets/texture/block/white_wool.png");
    whiteWool.SetBreakSound("dig.cloth");

    static Block lightGrayWool(true, "assets/texture/block/light_gray_wool.png");
    lightGrayWool.SetBreakSound("dig.cloth");

    static Block grayWool(true, "assets/texture/block/gray_wool.png");
    grayWool.SetBreakSound("dig.cloth");

    static Block blackWool(true, "assets/texture/block/black_wool.png");
    blackWool.SetBreakSound("dig.cloth");

    static Block brownWool(true, "assets/texture/block/brown_wool.png");
    brownWool.SetBreakSound("dig.cloth");

    static Block redWool(true, "assets/texture/block/red_wool.png");
    redWool.SetBreakSound("dig.cloth");

    static Block orangeWool(true, "assets/texture/block/orange_wool.png");
    orangeWool.SetBreakSound("dig.cloth");

    static Block yellowWool(true, "assets/texture/block/yellow_wool.png");
    yellowWool.SetBreakSound("dig.cloth");

    static Block limeWool(true, "assets/texture/block/lime_wool.png");
    limeWool.SetBreakSound("dig.cloth");

    static Block greenWool(true, "assets/texture/block/green_wool.png");
    greenWool.SetBreakSound("dig.cloth");

    static Block cyanWool(true, "assets/texture/block/cyan_wool.png");
    cyanWool.SetBreakSound("dig.cloth");

    static Block lightBlueWool(true, "assets/texture/block/light_blue_wool.png");
    lightBlueWool.SetBreakSound("dig.cloth");

    static Block blueWool(true, "assets/texture/block/blue_wool.png");
    blueWool.SetBreakSound("dig.cloth");

    static Block purpleWool(true, "assets/texture/block/purple_wool.png");
    purpleWool.SetBreakSound("dig.cloth");

    static Block magentaWool(true, "assets/texture/block/magenta_wool.png");
    magentaWool.SetBreakSound("dig.cloth");

    static Block pinkWool(true, "assets/texture/block/pink_wool.png");
    pinkWool.SetBreakSound("dig.cloth");

    static SlabBlock mossyStoneBrickSlab("assets/texture/block/mossy_stonebricks.png", &mossyStoneBricks);
    static StairBlock mossyStoneBrickStairs("assets/texture/block/mossy_stonebricks.png");
    static SlabBlock crackedStoneBrickSlab("assets/texture/block/cracked_stonebricks.png", &crackedStoneBricks);
    static StairBlock crackedStoneBrickStairs("assets/texture/block/cracked_stonebricks.png");
    static SlabBlock whiteBrickSlab("assets/texture/block/white_bricks.png", &whiteBricks);
    static StairBlock whiteBrickStairs("assets/texture/block/white_bricks.png");
    static SlabBlock lightGrayBrickSlab("assets/texture/block/light_gray_bricks.png", &lightGrayBricks);
    static StairBlock lightGrayBrickStairs("assets/texture/block/light_gray_bricks.png");
    static SlabBlock grayBrickSlab("assets/texture/block/gray_bricks.png", &grayBricks);
    static StairBlock grayBrickStairs("assets/texture/block/gray_bricks.png");
    static SlabBlock blackBrickSlab("assets/texture/block/black_bricks.png", &blackBricks);
    static StairBlock blackBrickStairs("assets/texture/block/black_bricks.png");
    static SlabBlock brownBrickSlab("assets/texture/block/brown_bricks.png", &brownBricks);
    static StairBlock brownBrickStairs("assets/texture/block/brown_bricks.png");
    static SlabBlock redBrickSlab("assets/texture/block/red_bricks.png", &redBricks);
    static StairBlock redBrickStairs("assets/texture/block/red_bricks.png");
    static SlabBlock vibrantRedBrickSlab("assets/texture/block/vibrant_red_bricks.png", &vibrantRedBricks);
    static StairBlock vibrantRedBrickStairs("assets/texture/block/vibrant_red_bricks.png");
    static SlabBlock orangeBrickSlab("assets/texture/block/orange_bricks.png", &orangeBricks);
    static StairBlock orangeBrickStairs("assets/texture/block/orange_bricks.png");
    static SlabBlock yellowBrickSlab("assets/texture/block/yellow_bricks.png", &yellowBricks);
    static StairBlock yellowBrickStairs("assets/texture/block/yellow_bricks.png");
    static SlabBlock limeBrickSlab("assets/texture/block/lime_bricks.png", &limeBricks);
    static StairBlock limeBrickStairs("assets/texture/block/lime_bricks.png");
    static SlabBlock greenBrickSlab("assets/texture/block/green_bricks.png", &greenBricks);
    static StairBlock greenBrickStairs("assets/texture/block/green_bricks.png");
    static SlabBlock cyanBrickSlab("assets/texture/block/cyan_bricks.png", &cyanBricks);
    static StairBlock cyanBrickStairs("assets/texture/block/cyan_bricks.png");
    static SlabBlock lightBlueBrickSlab("assets/texture/block/light_blue_bricks.png", &lightBlueBricks);
    static StairBlock lightBlueBrickStairs("assets/texture/block/light_blue_bricks.png");
    static SlabBlock blueBrickSlab("assets/texture/block/blue_bricks.png", &blueBricks);
    static StairBlock blueBrickStairs("assets/texture/block/blue_bricks.png");
    static SlabBlock purpleBrickSlab("assets/texture/block/purple_bricks.png", &purpleBricks);
    static StairBlock purpleBrickStairs("assets/texture/block/purple_bricks.png");
    static SlabBlock magentaBrickSlab("assets/texture/block/magenta_bricks.png", &magentaBricks);
    static StairBlock magentaBrickStairs("assets/texture/block/magenta_bricks.png");
    static SlabBlock pinkBrickSlab("assets/texture/block/pink_bricks.png", &pinkBricks);
    static StairBlock pinkBrickStairs("assets/texture/block/pink_bricks.png");

    static SlabBlock whiteWoolSlab("assets/texture/block/white_wool.png", &whiteWool);
    whiteWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock whiteWoolStairs("assets/texture/block/white_wool.png");
    whiteWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock lightGrayWoolSlab("assets/texture/block/light_gray_wool.png", &lightGrayWool);
    lightGrayWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock lightGrayWoolStairs("assets/texture/block/light_gray_wool.png");
    lightGrayWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock grayWoolSlab("assets/texture/block/gray_wool.png", &grayWool);
    grayWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock grayWoolStairs("assets/texture/block/gray_wool.png");
    grayWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock blackWoolSlab("assets/texture/block/black_wool.png", &blackWool);
    blackWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock blackWoolStairs("assets/texture/block/black_wool.png");
    blackWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock brownWoolSlab("assets/texture/block/brown_wool.png", &brownWool);
    brownWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock brownWoolStairs("assets/texture/block/brown_wool.png");
    brownWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock redWoolSlab("assets/texture/block/red_wool.png", &redWool);
    redWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock redWoolStairs("assets/texture/block/red_wool.png");
    redWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock orangeWoolSlab("assets/texture/block/orange_wool.png", &orangeWool);
    orangeWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock orangeWoolStairs("assets/texture/block/orange_wool.png");
    orangeWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock yellowWoolSlab("assets/texture/block/yellow_wool.png", &yellowWool);
    yellowWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock yellowWoolStairs("assets/texture/block/yellow_wool.png");
    yellowWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock limeWoolSlab("assets/texture/block/lime_wool.png", &limeWool);
    limeWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock limeWoolStairs("assets/texture/block/lime_wool.png");
    limeWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock greenWoolSlab("assets/texture/block/green_wool.png", &greenWool);
    greenWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock greenWoolStairs("assets/texture/block/green_wool.png");
    greenWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock cyanWoolSlab("assets/texture/block/cyan_wool.png", &cyanWool);
    cyanWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock cyanWoolStairs("assets/texture/block/cyan_wool.png");
    cyanWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock lightBlueWoolSlab("assets/texture/block/light_blue_wool.png", &lightBlueWool);
    lightBlueWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock lightBlueWoolStairs("assets/texture/block/light_blue_wool.png");
    lightBlueWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock blueWoolSlab("assets/texture/block/blue_wool.png", &blueWool);
    blueWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock blueWoolStairs("assets/texture/block/blue_wool.png");
    blueWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock purpleWoolSlab("assets/texture/block/purple_wool.png", &purpleWool);
    purpleWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock purpleWoolStairs("assets/texture/block/purple_wool.png");
    purpleWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock magentaWoolSlab("assets/texture/block/magenta_wool.png", &magentaWool);
    magentaWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock magentaWoolStairs("assets/texture/block/magenta_wool.png");
    magentaWoolStairs.SetBreakSound("dig.cloth");

    static SlabBlock pinkWoolSlab("assets/texture/block/pink_wool.png", &pinkWool);
    pinkWoolSlab.SetBreakSound("dig.cloth");

    static StairBlock pinkWoolStairs("assets/texture/block/pink_wool.png");
    pinkWoolStairs.SetBreakSound("dig.cloth");

    static Block whiteLamp(true, "assets/texture/block/white_lamp.png");
    whiteLamp.SetBreakSound("random.glass", 3);
    whiteLamp.SetPlaceSound("step.stone", 6, 0.8f);
    whiteLamp.SetLightEmission(15);
    whiteLamp.SetLightColor(0xFF, 0xFF, 0xFF);

    static Block yellowLamp(true, "assets/texture/block/yellow_lamp.png");
    yellowLamp.SetBreakSound("random.glass", 3);
    yellowLamp.SetPlaceSound("step.stone", 6, 0.8f);
    yellowLamp.SetLightEmission(15);
    yellowLamp.SetLightColor(0xF2, 0xCC, 0x74);

    static Block blueLamp(true, "assets/texture/block/blue_lamp.png");
    blueLamp.SetBreakSound("random.glass", 3);
    blueLamp.SetPlaceSound("step.stone", 6, 0.8f);
    blueLamp.SetLightEmission(15);
    blueLamp.SetLightColor(0x74, 0xBA, 0xF2);

    static Block cyanLamp(true, "assets/texture/block/cyan_lamp.png");
    cyanLamp.SetBreakSound("random.glass", 3);
    cyanLamp.SetPlaceSound("step.stone", 6, 0.8f);
    cyanLamp.SetLightEmission(15);
    cyanLamp.SetLightColor(0x74, 0xE0, 0xF2);

    static Block greenLamp(true, "assets/texture/block/green_lamp.png");
    greenLamp.SetBreakSound("random.glass", 3);
    greenLamp.SetPlaceSound("step.stone", 6, 0.8f);
    greenLamp.SetLightEmission(15);
    greenLamp.SetLightColor(0x81, 0xF2, 0x74);

    static Block pinkLamp(true, "assets/texture/block/pink_lamp.png");
    pinkLamp.SetBreakSound("random.glass", 3);
    pinkLamp.SetPlaceSound("step.stone", 6, 0.8f);
    pinkLamp.SetLightEmission(15);
    pinkLamp.SetLightColor(0xE5, 0x74, 0xF1);

    static Block purpleLamp(true, "assets/texture/block/purple_lamp.png");
    purpleLamp.SetBreakSound("random.glass", 3);
    purpleLamp.SetPlaceSound("step.stone", 6, 0.8f);
    purpleLamp.SetLightEmission(15);
    purpleLamp.SetLightColor(0x95, 0x74, 0xF1);

    static Block redLamp(true, "assets/texture/block/red_lamp.png");
    redLamp.SetBreakSound("random.glass", 3);
    redLamp.SetPlaceSound("step.stone", 6, 0.8f);
    redLamp.SetLightEmission(15);
    redLamp.SetLightColor(0xF2, 0x7F, 0x63);

    registry->Register("air", &air);
    registry->Register("bedrock", &bedrock);
    registry->Register("stone", &stone);
    registry->Register("cobblestone", &cobblestone);
    registry->Register("mossy_cobblestone", &mossyCobblestone);
    registry->Register("obsidian", &obsidian);
    registry->Register("andesite", &andesite);
    registry->Register("stone_bricks", &stoneBricks);
    registry->Register("mossy_stone_bricks", &mossyStoneBricks);
    registry->Register("cracked_stone_bricks", &crackedStoneBricks);
    registry->Register("chiseled_stone_bricks", &chiseledStoneBricks);
    registry->Register("stone_slab", &stoneSlab);
    registry->Register("stone_stairs", &stoneStairs);
    registry->Register("stone_brick_slab", &stoneBrickSlab);
    registry->Register("stone_brick_stairs", &stoneBrickStairs);
    registry->Register("dirt", &dirt);
    registry->Register("grass_block", &grass_block);
    registry->Register("grass", &grass);
    registry->Register("fern", &fern);
    registry->Register("sand", &sand);
    registry->Register("gravel", &gravel);
    registry->Register("water_still", &waterStill);
    registry->Register("water_flow", &waterFlowBlock);
    registry->Register("lava_still", &lavaStill);
    registry->Register("lava_flow", &lavaFlowBlock);
    registry->Register("oak_log", &oakLog);
    registry->Register("spruce_log", &spruceLog);
    registry->Register("oak_planks", &oakPlanks);
    registry->Register("spruce_planks", &sprucePlanks);
    registry->Register("oak_slab", &oakSlab);
    registry->Register("oak_stairs", &oakStairs);
    registry->Register("spruce_slab", &spruceSlab);
    registry->Register("spruce_stairs", &spruceStairs);
    registry->Register("oak_leaves", &oakLeaves);
    registry->Register("spruce_leaves", &spruceLeaves);
    registry->Register("crafting_table", &craftingTable);
    registry->Register("torch", &torch);
    registry->Register("torch_wall", &torchWall);
    registry->Register("white_lamp", &whiteLamp);
    registry->Register("red_lamp", &redLamp);
    registry->Register("yellow_lamp", &yellowLamp);
    registry->Register("green_lamp", &greenLamp);
    registry->Register("cyan_lamp", &cyanLamp);
    registry->Register("blue_lamp", &blueLamp);
    registry->Register("purple_lamp", &purpleLamp);
    registry->Register("pink_lamp", &pinkLamp);
    registry->Register("checkered_tiles", &checkeredTiles);
    registry->Register("white_bricks", &whiteBricks);
    registry->Register("light_gray_bricks", &lightGrayBricks);
    registry->Register("gray_bricks", &grayBricks);
    registry->Register("black_bricks", &blackBricks);
    registry->Register("brown_bricks", &brownBricks);
    registry->Register("red_bricks", &redBricks);
    registry->Register("vibrant_red_bricks", &vibrantRedBricks);
    registry->Register("orange_bricks", &orangeBricks);
    registry->Register("yellow_bricks", &yellowBricks);
    registry->Register("lime_bricks", &limeBricks);
    registry->Register("green_bricks", &greenBricks);
    registry->Register("cyan_bricks", &cyanBricks);
    registry->Register("light_blue_bricks", &lightBlueBricks);
    registry->Register("blue_bricks", &blueBricks);
    registry->Register("purple_bricks", &purpleBricks);
    registry->Register("magenta_bricks", &magentaBricks);
    registry->Register("pink_bricks", &pinkBricks);
    registry->Register("white_wool", &whiteWool);
    registry->Register("light_gray_wool", &lightGrayWool);
    registry->Register("gray_wool", &grayWool);
    registry->Register("black_wool", &blackWool);
    registry->Register("brown_wool", &brownWool);
    registry->Register("red_wool", &redWool);
    registry->Register("orange_wool", &orangeWool);
    registry->Register("yellow_wool", &yellowWool);
    registry->Register("lime_wool", &limeWool);
    registry->Register("green_wool", &greenWool);
    registry->Register("cyan_wool", &cyanWool);
    registry->Register("light_blue_wool", &lightBlueWool);
    registry->Register("blue_wool", &blueWool);
    registry->Register("purple_wool", &purpleWool);
    registry->Register("magenta_wool", &magentaWool);
    registry->Register("pink_wool", &pinkWool);
    registry->Register("granite", &granite);
    registry->Register("polished_granite", &polishedGranite);
    registry->Register("diorite", &diorite);
    registry->Register("polished_diorite", &polishedDiorite);
    registry->Register("polished_andesite", &polishedAndesite);
    registry->Register("birch_log", &birchLog);
    registry->Register("acacia_log", &acaciaLog);
    registry->Register("birch_planks", &birchPlanks);
    registry->Register("acacia_planks", &acaciaPlanks);
    registry->Register("birch_leaves", &birchLeaves);
    registry->Register("acacia_leaves", &acaciaLeaves);
    registry->Register("snow_block", &snowBlock);
    registry->Register("snow", &snow);
    registry->Register("mossy_stone_brick_slab", &mossyStoneBrickSlab);
    registry->Register("mossy_stone_brick_stairs", &mossyStoneBrickStairs);
    registry->Register("cracked_stone_brick_slab", &crackedStoneBrickSlab);
    registry->Register("cracked_stone_brick_stairs", &crackedStoneBrickStairs);
    registry->Register("white_brick_slab", &whiteBrickSlab);
    registry->Register("white_brick_stairs", &whiteBrickStairs);
    registry->Register("light_gray_brick_slab", &lightGrayBrickSlab);
    registry->Register("light_gray_brick_stairs", &lightGrayBrickStairs);
    registry->Register("gray_brick_slab", &grayBrickSlab);
    registry->Register("gray_brick_stairs", &grayBrickStairs);
    registry->Register("black_brick_slab", &blackBrickSlab);
    registry->Register("black_brick_stairs", &blackBrickStairs);
    registry->Register("brown_brick_slab", &brownBrickSlab);
    registry->Register("brown_brick_stairs", &brownBrickStairs);
    registry->Register("red_brick_slab", &redBrickSlab);
    registry->Register("red_brick_stairs", &redBrickStairs);
    registry->Register("vibrant_red_brick_slab", &vibrantRedBrickSlab);
    registry->Register("vibrant_red_brick_stairs", &vibrantRedBrickStairs);
    registry->Register("orange_brick_slab", &orangeBrickSlab);
    registry->Register("orange_brick_stairs", &orangeBrickStairs);
    registry->Register("yellow_brick_slab", &yellowBrickSlab);
    registry->Register("yellow_brick_stairs", &yellowBrickStairs);
    registry->Register("lime_brick_slab", &limeBrickSlab);
    registry->Register("lime_brick_stairs", &limeBrickStairs);
    registry->Register("green_brick_slab", &greenBrickSlab);
    registry->Register("green_brick_stairs", &greenBrickStairs);
    registry->Register("cyan_brick_slab", &cyanBrickSlab);
    registry->Register("cyan_brick_stairs", &cyanBrickStairs);
    registry->Register("light_blue_brick_slab", &lightBlueBrickSlab);
    registry->Register("light_blue_brick_stairs", &lightBlueBrickStairs);
    registry->Register("blue_brick_slab", &blueBrickSlab);
    registry->Register("blue_brick_stairs", &blueBrickStairs);
    registry->Register("purple_brick_slab", &purpleBrickSlab);
    registry->Register("purple_brick_stairs", &purpleBrickStairs);
    registry->Register("magenta_brick_slab", &magentaBrickSlab);
    registry->Register("magenta_brick_stairs", &magentaBrickStairs);
    registry->Register("pink_brick_slab", &pinkBrickSlab);
    registry->Register("pink_brick_stairs", &pinkBrickStairs);
    registry->Register("white_wool_slab", &whiteWoolSlab);
    registry->Register("white_wool_stairs", &whiteWoolStairs);
    registry->Register("light_gray_wool_slab", &lightGrayWoolSlab);
    registry->Register("light_gray_wool_stairs", &lightGrayWoolStairs);
    registry->Register("gray_wool_slab", &grayWoolSlab);
    registry->Register("gray_wool_stairs", &grayWoolStairs);
    registry->Register("black_wool_slab", &blackWoolSlab);
    registry->Register("black_wool_stairs", &blackWoolStairs);
    registry->Register("brown_wool_slab", &brownWoolSlab);
    registry->Register("brown_wool_stairs", &brownWoolStairs);
    registry->Register("red_wool_slab", &redWoolSlab);
    registry->Register("red_wool_stairs", &redWoolStairs);
    registry->Register("orange_wool_slab", &orangeWoolSlab);
    registry->Register("orange_wool_stairs", &orangeWoolStairs);
    registry->Register("yellow_wool_slab", &yellowWoolSlab);
    registry->Register("yellow_wool_stairs", &yellowWoolStairs);
    registry->Register("lime_wool_slab", &limeWoolSlab);
    registry->Register("lime_wool_stairs", &limeWoolStairs);
    registry->Register("green_wool_slab", &greenWoolSlab);
    registry->Register("green_wool_stairs", &greenWoolStairs);
    registry->Register("cyan_wool_slab", &cyanWoolSlab);
    registry->Register("cyan_wool_stairs", &cyanWoolStairs);
    registry->Register("light_blue_wool_slab", &lightBlueWoolSlab);
    registry->Register("light_blue_wool_stairs", &lightBlueWoolStairs);
    registry->Register("blue_wool_slab", &blueWoolSlab);
    registry->Register("blue_wool_stairs", &blueWoolStairs);
    registry->Register("purple_wool_slab", &purpleWoolSlab);
    registry->Register("purple_wool_stairs", &purpleWoolStairs);
    registry->Register("magenta_wool_slab", &magentaWoolSlab);
    registry->Register("magenta_wool_stairs", &magentaWoolStairs);
    registry->Register("pink_wool_slab", &pinkWoolSlab);
    registry->Register("pink_wool_stairs", &pinkWoolStairs);

    static ClimbableBlock ladder("assets/texture/block/ladder.png");
    ladder.SetBreakSound("dig.wood");
    registry->Register("ladder", &ladder);

    static VineBlock vine("assets/texture/block/vine.png");
    vine.SetBreakSound("dig.grass");
    vine.SetFoliageTint();
    registry->Register("vine", &vine);

    static DoorBlock oakDoor("assets/texture/block/door_wood_lower.png", "assets/texture/block/door_wood_upper.png");
    oakDoor.SetBreakSound("dig.wood");
    registry->Register("oak_door", &oakDoor);

    static DoorBlock spruceDoor("assets/texture/block/door_spruce_lower.png", "assets/texture/block/door_spruce_upper.png");
    spruceDoor.SetBreakSound("dig.wood");
    registry->Register("spruce_door", &spruceDoor);

    static DoorBlock birchDoor("assets/texture/block/door_birch_lower.png", "assets/texture/block/door_birch_upper.png");
    birchDoor.SetBreakSound("dig.wood");
    registry->Register("birch_door", &birchDoor);

    static DoorBlock acaciaDoor("assets/texture/block/door_acacia_lower.png", "assets/texture/block/door_acacia_upper.png");
    acaciaDoor.SetBreakSound("dig.wood");
    registry->Register("acacia_door", &acaciaDoor);

    static FenceBlock oakFence("assets/texture/block/planks_oak.png");
    registry->Register("oak_fence", &oakFence);

    static FenceGateBlock oakFenceGate("assets/texture/block/planks_oak.png");
    registry->Register("oak_fence_gate", &oakFenceGate);

    static ButtonBlock oakButton("assets/texture/block/planks_oak.png", 30);
    oakButton.SetBreakSound("dig.wood");
    registry->Register("oak_button", &oakButton);

    static PressurePlateBlock oakPressurePlate("assets/texture/block/planks_oak.png");
    oakPressurePlate.SetBreakSound("dig.wood");
    registry->Register("oak_pressure_plate", &oakPressurePlate);

    static FenceBlock spruceFence("assets/texture/block/planks_spruce.png");
    registry->Register("spruce_fence", &spruceFence);

    static FenceGateBlock spruceFenceGate("assets/texture/block/planks_spruce.png");
    registry->Register("spruce_fence_gate", &spruceFenceGate);

    static ButtonBlock spruceButton("assets/texture/block/planks_spruce.png", 30);
    spruceButton.SetBreakSound("dig.wood");
    registry->Register("spruce_button", &spruceButton);

    static PressurePlateBlock sprucePressurePlate("assets/texture/block/planks_spruce.png");
    sprucePressurePlate.SetBreakSound("dig.wood");
    registry->Register("spruce_pressure_plate", &sprucePressurePlate);

    static FenceBlock birchFence("assets/texture/block/birch_planks.png");
    registry->Register("birch_fence", &birchFence);

    static FenceGateBlock birchFenceGate("assets/texture/block/birch_planks.png");
    registry->Register("birch_fence_gate", &birchFenceGate);

    static ButtonBlock birchButton("assets/texture/block/birch_planks.png", 30);
    birchButton.SetBreakSound("dig.wood");
    registry->Register("birch_button", &birchButton);

    static PressurePlateBlock birchPressurePlate("assets/texture/block/birch_planks.png");
    birchPressurePlate.SetBreakSound("dig.wood");
    registry->Register("birch_pressure_plate", &birchPressurePlate);

    static FenceBlock acaciaFence("assets/texture/block/acacia_planks.png");
    registry->Register("acacia_fence", &acaciaFence);

    static FenceGateBlock acaciaFenceGate("assets/texture/block/acacia_planks.png");
    registry->Register("acacia_fence_gate", &acaciaFenceGate);

    static ButtonBlock acaciaButton("assets/texture/block/acacia_planks.png", 30);
    acaciaButton.SetBreakSound("dig.wood");
    registry->Register("acacia_button", &acaciaButton);

    static PressurePlateBlock acaciaPressurePlate("assets/texture/block/acacia_planks.png");
    acaciaPressurePlate.SetBreakSound("dig.wood");
    registry->Register("acacia_pressure_plate", &acaciaPressurePlate);

    static ButtonBlock stoneButton("assets/texture/block/stone.png", 20);
    registry->Register("stone_button", &stoneButton);

    static PressurePlateBlock stonePressurePlate("assets/texture/block/stone.png");
    registry->Register("stone_pressure_plate", &stonePressurePlate);

    static LeverBlock lever;
    lever.SetBreakSound("dig.wood");
    registry->Register("lever", &lever);

    static Block glass(true, "assets/texture/block/glass.png");
    glass.SetTransparent(true);
    glass.SetBreakSound("random.glass", 3);
    glass.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("glass", &glass);

    static PaneBlock glassPane("assets/texture/block/glass.png", "assets/texture/block/glass_pane_edge.png");
    registry->Register("glass_pane", &glassPane);

    static Block whiteStainedGlass(true, "assets/texture/block/glass_white.png");
    whiteStainedGlass.SetTransparent(true);
    whiteStainedGlass.SetBreakSound("random.glass", 3);
    whiteStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    whiteStainedGlass.SetTranslucent(true);
    registry->Register("white_stained_glass", &whiteStainedGlass);

    static PaneBlock whiteStainedGlassPane("assets/texture/block/glass_white.png", "assets/texture/block/glass_pane_edge_white.png");
    whiteStainedGlassPane.SetTranslucent(true);
    registry->Register("white_stained_glass_pane", &whiteStainedGlassPane);

    static Block lightGrayStainedGlass(true, "assets/texture/block/glass_silver.png");
    lightGrayStainedGlass.SetTransparent(true);
    lightGrayStainedGlass.SetBreakSound("random.glass", 3);
    lightGrayStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    lightGrayStainedGlass.SetTranslucent(true);
    registry->Register("light_gray_stained_glass", &lightGrayStainedGlass);

    static PaneBlock lightGrayStainedGlassPane("assets/texture/block/glass_silver.png", "assets/texture/block/glass_pane_edge_silver.png");
    lightGrayStainedGlassPane.SetTranslucent(true);
    registry->Register("light_gray_stained_glass_pane", &lightGrayStainedGlassPane);

    static Block grayStainedGlass(true, "assets/texture/block/glass_gray.png");
    grayStainedGlass.SetTransparent(true);
    grayStainedGlass.SetBreakSound("random.glass", 3);
    grayStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    grayStainedGlass.SetTranslucent(true);
    registry->Register("gray_stained_glass", &grayStainedGlass);

    static PaneBlock grayStainedGlassPane("assets/texture/block/glass_gray.png", "assets/texture/block/glass_pane_edge_gray.png");
    grayStainedGlassPane.SetTranslucent(true);
    registry->Register("gray_stained_glass_pane", &grayStainedGlassPane);

    static Block blackStainedGlass(true, "assets/texture/block/glass_black.png");
    blackStainedGlass.SetTransparent(true);
    blackStainedGlass.SetBreakSound("random.glass", 3);
    blackStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    blackStainedGlass.SetTranslucent(true);
    registry->Register("black_stained_glass", &blackStainedGlass);

    static PaneBlock blackStainedGlassPane("assets/texture/block/glass_black.png", "assets/texture/block/glass_pane_edge_black.png");
    blackStainedGlassPane.SetTranslucent(true);
    registry->Register("black_stained_glass_pane", &blackStainedGlassPane);

    static Block brownStainedGlass(true, "assets/texture/block/glass_brown.png");
    brownStainedGlass.SetTransparent(true);
    brownStainedGlass.SetBreakSound("random.glass", 3);
    brownStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    brownStainedGlass.SetTranslucent(true);
    registry->Register("brown_stained_glass", &brownStainedGlass);

    static PaneBlock brownStainedGlassPane("assets/texture/block/glass_brown.png", "assets/texture/block/glass_pane_edge_brown.png");
    brownStainedGlassPane.SetTranslucent(true);
    registry->Register("brown_stained_glass_pane", &brownStainedGlassPane);

    static Block redStainedGlass(true, "assets/texture/block/glass_red.png");
    redStainedGlass.SetTransparent(true);
    redStainedGlass.SetBreakSound("random.glass", 3);
    redStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    redStainedGlass.SetTranslucent(true);
    registry->Register("red_stained_glass", &redStainedGlass);

    static PaneBlock redStainedGlassPane("assets/texture/block/glass_red.png", "assets/texture/block/glass_pane_edge_red.png");
    redStainedGlassPane.SetTranslucent(true);
    registry->Register("red_stained_glass_pane", &redStainedGlassPane);

    static Block orangeStainedGlass(true, "assets/texture/block/glass_orange.png");
    orangeStainedGlass.SetTransparent(true);
    orangeStainedGlass.SetBreakSound("random.glass", 3);
    orangeStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    orangeStainedGlass.SetTranslucent(true);
    registry->Register("orange_stained_glass", &orangeStainedGlass);

    static PaneBlock orangeStainedGlassPane("assets/texture/block/glass_orange.png", "assets/texture/block/glass_pane_edge_orange.png");
    orangeStainedGlassPane.SetTranslucent(true);
    registry->Register("orange_stained_glass_pane", &orangeStainedGlassPane);

    static Block yellowStainedGlass(true, "assets/texture/block/glass_yellow.png");
    yellowStainedGlass.SetTransparent(true);
    yellowStainedGlass.SetBreakSound("random.glass", 3);
    yellowStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    yellowStainedGlass.SetTranslucent(true);
    registry->Register("yellow_stained_glass", &yellowStainedGlass);

    static PaneBlock yellowStainedGlassPane("assets/texture/block/glass_yellow.png", "assets/texture/block/glass_pane_edge_yellow.png");
    yellowStainedGlassPane.SetTranslucent(true);
    registry->Register("yellow_stained_glass_pane", &yellowStainedGlassPane);

    static Block limeStainedGlass(true, "assets/texture/block/glass_lime.png");
    limeStainedGlass.SetTransparent(true);
    limeStainedGlass.SetBreakSound("random.glass", 3);
    limeStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    limeStainedGlass.SetTranslucent(true);
    registry->Register("lime_stained_glass", &limeStainedGlass);

    static PaneBlock limeStainedGlassPane("assets/texture/block/glass_lime.png", "assets/texture/block/glass_pane_edge_lime.png");
    limeStainedGlassPane.SetTranslucent(true);
    registry->Register("lime_stained_glass_pane", &limeStainedGlassPane);

    static Block greenStainedGlass(true, "assets/texture/block/glass_green.png");
    greenStainedGlass.SetTransparent(true);
    greenStainedGlass.SetBreakSound("random.glass", 3);
    greenStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    greenStainedGlass.SetTranslucent(true);
    registry->Register("green_stained_glass", &greenStainedGlass);

    static PaneBlock greenStainedGlassPane("assets/texture/block/glass_green.png", "assets/texture/block/glass_pane_edge_green.png");
    greenStainedGlassPane.SetTranslucent(true);
    registry->Register("green_stained_glass_pane", &greenStainedGlassPane);

    static Block cyanStainedGlass(true, "assets/texture/block/glass_cyan.png");
    cyanStainedGlass.SetTransparent(true);
    cyanStainedGlass.SetBreakSound("random.glass", 3);
    cyanStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    cyanStainedGlass.SetTranslucent(true);
    registry->Register("cyan_stained_glass", &cyanStainedGlass);

    static PaneBlock cyanStainedGlassPane("assets/texture/block/glass_cyan.png", "assets/texture/block/glass_pane_edge_cyan.png");
    cyanStainedGlassPane.SetTranslucent(true);
    registry->Register("cyan_stained_glass_pane", &cyanStainedGlassPane);

    static Block lightBlueStainedGlass(true, "assets/texture/block/glass_light_blue.png");
    lightBlueStainedGlass.SetTransparent(true);
    lightBlueStainedGlass.SetBreakSound("random.glass", 3);
    lightBlueStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    lightBlueStainedGlass.SetTranslucent(true);
    registry->Register("light_blue_stained_glass", &lightBlueStainedGlass);

    static PaneBlock lightBlueStainedGlassPane("assets/texture/block/glass_light_blue.png", "assets/texture/block/glass_pane_edge_light_blue.png");
    lightBlueStainedGlassPane.SetTranslucent(true);
    registry->Register("light_blue_stained_glass_pane", &lightBlueStainedGlassPane);

    static Block blueStainedGlass(true, "assets/texture/block/glass_blue.png");
    blueStainedGlass.SetTransparent(true);
    blueStainedGlass.SetBreakSound("random.glass", 3);
    blueStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    blueStainedGlass.SetTranslucent(true);
    registry->Register("blue_stained_glass", &blueStainedGlass);

    static PaneBlock blueStainedGlassPane("assets/texture/block/glass_blue.png", "assets/texture/block/glass_pane_edge_blue.png");
    blueStainedGlassPane.SetTranslucent(true);
    registry->Register("blue_stained_glass_pane", &blueStainedGlassPane);

    static Block purpleStainedGlass(true, "assets/texture/block/glass_purple.png");
    purpleStainedGlass.SetTransparent(true);
    purpleStainedGlass.SetBreakSound("random.glass", 3);
    purpleStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    purpleStainedGlass.SetTranslucent(true);
    registry->Register("purple_stained_glass", &purpleStainedGlass);

    static PaneBlock purpleStainedGlassPane("assets/texture/block/glass_purple.png", "assets/texture/block/glass_pane_edge_purple.png");
    purpleStainedGlassPane.SetTranslucent(true);
    registry->Register("purple_stained_glass_pane", &purpleStainedGlassPane);

    static Block magentaStainedGlass(true, "assets/texture/block/glass_magenta.png");
    magentaStainedGlass.SetTransparent(true);
    magentaStainedGlass.SetBreakSound("random.glass", 3);
    magentaStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    magentaStainedGlass.SetTranslucent(true);
    registry->Register("magenta_stained_glass", &magentaStainedGlass);

    static PaneBlock magentaStainedGlassPane("assets/texture/block/glass_magenta.png", "assets/texture/block/glass_pane_edge_magenta.png");
    magentaStainedGlassPane.SetTranslucent(true);
    registry->Register("magenta_stained_glass_pane", &magentaStainedGlassPane);

    static Block pinkStainedGlass(true, "assets/texture/block/glass_pink.png");
    pinkStainedGlass.SetTransparent(true);
    pinkStainedGlass.SetBreakSound("random.glass", 3);
    pinkStainedGlass.SetPlaceSound("step.stone", 6, 0.8f);
    pinkStainedGlass.SetTranslucent(true);
    registry->Register("pink_stained_glass", &pinkStainedGlass);

    static PaneBlock pinkStainedGlassPane("assets/texture/block/glass_pink.png", "assets/texture/block/glass_pane_edge_pink.png");
    pinkStainedGlassPane.SetTranslucent(true);
    registry->Register("pink_stained_glass_pane", &pinkStainedGlassPane);

    static FireBlock fire("assets/texture/block/fire_layer_0.png", "assets/texture/block/fire_layer_1.png");
    registry->Register("fire", &fire);

    static IceBlock ice("assets/texture/block/ice.png");
    ice.SetBreakSound("random.glass", 3);
    ice.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice", &ice);

    static Block packedIce(true, "assets/texture/block/packed_ice.png");
    packedIce.SetSlipperiness(0.98f);
    packedIce.SetBreakSound("random.glass", 3);
    packedIce.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("packed_ice", &packedIce);

    static Block dripstoneBlock(true, "assets/texture/block/dripstone_block.png");
    dripstoneBlock.SetBreakSound("dripstone.break", 5);
    dripstoneBlock.SetPlaceSound("dripstone.break", 5, 0.8f);
    registry->Register("dripstone_block", &dripstoneBlock);

    static PointedDripstoneBlock pointedDripstone("assets/texture/block/pointed_dripstone");
    pointedDripstone.SetBreakSound("dripstone.break", 5);
    pointedDripstone.SetPlaceSound("dripstone.break", 5, 0.8f);
    registry->Register("pointed_dripstone", &pointedDripstone);

    static EnchantedIceBlock enchantedIce("assets/texture/block/enchanted_ice.png");
    enchantedIce.SetBreakSound("random.glass", 3);
    enchantedIce.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("enchanted_ice", &enchantedIce);

    static Block resonantCrystal(true, "assets/texture/block/resonant_crystal.png");
    resonantCrystal.SetTransparent(true);
    resonantCrystal.SetTranslucent(true);
    resonantCrystal.SetBreakSound("random.glass", 3);
    resonantCrystal.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("resonant_crystal", &resonantCrystal);

    static Block sealedFractureFrame(true, "assets/texture/block/sealed_fracture_frame.png");
    registry->Register("sealed_fracture_frame", &sealedFractureFrame);
    static Block crackedFractureFrame(true, "assets/texture/block/cracked_fracture_frame.png");
    registry->Register("cracked_fracture_frame", &crackedFractureFrame);
    static Block fracturedFractureFrame(true, "assets/texture/block/fractured_fracture_frame.png");
    registry->Register("fractured_fracture_frame", &fracturedFractureFrame);
    static Block brokenFractureFrame(true, "assets/texture/block/broken_fracture_frame.png");
    registry->Register("broken_fracture_frame", &brokenFractureFrame);
    static ResonantSocketBlock resonantSocket;
    registry->Register("resonant_socket", &resonantSocket);
    static FractureCoreBlock fractureCore;
    registry->Register("fracture_core", &fractureCore);

    static Block echoQuartz(true, "assets/texture/block/echo_quartz.png");
    echoQuartz.SetLightEmission(6);
    echoQuartz.SetLightColor(0xA0, 0xE8, 0xE8);
    registry->Register("echo_quartz", &echoQuartz);
    static Block polishedEchoQuartz(true, "assets/texture/block/polished_echo_quartz.png");
    registry->Register("polished_echo_quartz", &polishedEchoQuartz);
    static Block echoQuartzBricks(true, "assets/texture/block/echo_quartz_bricks.png");
    registry->Register("echo_quartz_bricks", &echoQuartzBricks);
    static Block echoQuartzPillar(true, "assets/texture/block/echo_quartz_pillar.png");
    echoQuartzPillar.SetPillar("assets/texture/block/echo_quartz_pillar_top.png");
    registry->Register("echo_quartz_pillar", &echoQuartzPillar);
    static EchoGlassBlock echoGlass("assets/texture/block/echo_glass.png", "assets/texture/block/echo_glass_solid.png");
    echoGlass.SetTransparent(true);
    echoGlass.SetTranslucent(true);
    echoGlass.SetBreakSound("random.glass", 3);
    echoGlass.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("echo_glass", &echoGlass);
    static Block hollowCore(true, "assets/texture/block/hollow_core.png");
    registry->Register("hollow_core", &hollowCore);
    static ResonatorBlock resonator;
    registry->Register("resonator", &resonator);
    static AnvilBlock anvil("assets/texture/block/anvil_top.png", "chipped_anvil");
    registry->Register("anvil", &anvil);
    static AnvilBlock chippedAnvil("assets/texture/block/chipped_anvil_top.png", "damaged_anvil");
    registry->Register("chipped_anvil", &chippedAnvil);
    static AnvilBlock damagedAnvil("assets/texture/block/damaged_anvil_top.png", "");
    registry->Register("damaged_anvil", &damagedAnvil);
    static Block richResonantCrystal(true, "assets/texture/block/rich_resonant_crystal.png");
    richResonantCrystal.SetTransparent(true);
    richResonantCrystal.SetTranslucent(true);
    richResonantCrystal.SetBreakSound("random.glass", 3);
    richResonantCrystal.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("rich_resonant_crystal", &richResonantCrystal);

    static PointedDripstoneBlock icicle("assets/texture/block/icicle", true);
    icicle.SetBreakSound("random.glass", 3);
    icicle.SetPlaceSound("step.stone", 6, 0.8f);
    icicle.SetLandSound("random.glass", 3);
    registry->Register("icicle", &icicle);

    static IceSheetBlock iceSheet("assets/texture/block/ice_sheet.png");
    iceSheet.SetBreakSound("random.glass", 3);
    iceSheet.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_sheet", &iceSheet);

    static Block magma(true, "assets/texture/block/magma.png");
    magma.SetLightEmission(3);
    magma.SetLightColor(0xFF, 0x70, 0x30);
    registry->Register("magma", &magma);

    static Block sandstone(true, "assets/texture/block/sandstone.png");
    sandstone.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_top.png");
    sandstone.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_bottom.png");
    registry->Register("sandstone", &sandstone);

    static Block chiseledSandstone(true, "assets/texture/block/chiseled_sandstone.png");
    chiseledSandstone.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_top.png");
    chiseledSandstone.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_top.png");
    registry->Register("chiseled_sandstone", &chiseledSandstone);

    static Block cutSandstone(true, "assets/texture/block/cut_sandstone.png");
    cutSandstone.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_top.png");
    cutSandstone.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_top.png");
    registry->Register("cut_sandstone", &cutSandstone);

    static Block smoothSandstone(true, "assets/texture/block/sandstone_top.png");
    registry->Register("smooth_sandstone", &smoothSandstone);

    static Block redSandstone(true, "assets/texture/block/red_sandstone.png");
    redSandstone.SetFaceTexture(Direction::up, "assets/texture/block/red_sandstone_top.png");
    redSandstone.SetFaceTexture(Direction::down, "assets/texture/block/red_sandstone_bottom.png");
    registry->Register("red_sandstone", &redSandstone);

    static Block chiseledRedSandstone(true, "assets/texture/block/chiseled_red_sandstone.png");
    chiseledRedSandstone.SetFaceTexture(Direction::up, "assets/texture/block/red_sandstone_top.png");
    chiseledRedSandstone.SetFaceTexture(Direction::down, "assets/texture/block/red_sandstone_top.png");
    registry->Register("chiseled_red_sandstone", &chiseledRedSandstone);

    static Block cutRedSandstone(true, "assets/texture/block/cut_red_sandstone.png");
    cutRedSandstone.SetFaceTexture(Direction::up, "assets/texture/block/red_sandstone_top.png");
    cutRedSandstone.SetFaceTexture(Direction::down, "assets/texture/block/red_sandstone_top.png");
    registry->Register("cut_red_sandstone", &cutRedSandstone);

    static Block smoothRedSandstone(true, "assets/texture/block/red_sandstone_top.png");
    registry->Register("smooth_red_sandstone", &smoothRedSandstone);

    static GravityBlock redSand("assets/texture/block/red_sand.png");
    redSand.SetBreakSound("dig.sand");
    registry->Register("red_sand", &redSand);

    static CactusBlock cactus("assets/texture/block/cactus_side.png", "assets/texture/block/cactus_top.png",
                              "assets/texture/block/cactus_bottom.png");
    cactus.SetBreakSound("dig.cloth");
    registry->Register("cactus", &cactus);

    static DeadBushBlock deadBush("assets/texture/block/dead_bush.png");
    deadBush.SetBreakSound("dig.grass");
    registry->Register("dead_bush", &deadBush);

    static SugarCaneBlock sugarCane("assets/texture/block/sugar_cane.png");
    sugarCane.SetBreakSound("dig.grass");
    registry->Register("sugar_cane", &sugarCane);

    static DoublePlantBlock tallGrass("assets/texture/block/tall_grass_bottom.png", "assets/texture/block/tall_grass_top.png");
    tallGrass.SetBiomeTint();
    tallGrass.SetReplaceable(true);
    registry->Register("tall_grass", &tallGrass);

    static DoublePlantBlock largeFern("assets/texture/block/large_fern_bottom.png", "assets/texture/block/large_fern_top.png");
    largeFern.SetBiomeTint();
    largeFern.SetReplaceable(true);
    registry->Register("large_fern", &largeFern);

    static DoublePlantBlock lilac("assets/texture/block/lilac_bottom.png", "assets/texture/block/lilac_top.png");
    registry->Register("lilac", &lilac);

    static DoublePlantBlock roseBush("assets/texture/block/rose_bush_bottom.png", "assets/texture/block/rose_bush_top.png");
    registry->Register("rose_bush", &roseBush);

    static DoublePlantBlock peony("assets/texture/block/peony_bottom.png", "assets/texture/block/peony_top.png");
    registry->Register("peony", &peony);

    const char *flowerNames[] = {"dandelion", "poppy",        "blue_orchid", "allium",     "azure_bluet",
                                 "red_tulip", "orange_tulip", "white_tulip", "pink_tulip", "oxeye_daisy"};
    static std::vector<std::unique_ptr<Block>> flowers;
    for (size_t i = 0; i < sizeof(flowerNames) / sizeof(flowerNames[0]); i++)
    {
        std::unique_ptr<Block> flower = std::make_unique<Block>(false, std::string("assets/texture/block/") + flowerNames[i] + ".png");
        flower->SetBreakSound("dig.grass");
        flower->SetCollidable(false);
        flower->SetRenderShape(Block::eRenderShape_CROSS);
        flower->SetCrossJitter(Block::eCrossJitter_Horizontal);
        flower->SetAABB(AABB(glm::dvec3(0.3, 0.0, 0.3), glm::dvec3(0.7, 0.6, 0.7)));
        flower->SetNeedsSupportBelow(true);
        flower->SetSupportBlocks({"grass_block", "dirt", "coarse_dirt", "podzol"});
        flower->SetFlammable(60, 100);
        registry->Register(flowerNames[i], flower.get());
        flowers.push_back(std::move(flower));
    }

    static SunbloomBlock sunbloom("assets/texture/block/sunbloom.png");
    sunbloom.SetBreakSound("dig.grass");
    sunbloom.SetFlammable(60, 100);
    sunbloom.SetLightEmission(10);
    sunbloom.SetLightColor(0xFF, 0xC8, 0x40);
    registry->Register("sunbloom", &sunbloom);

    static SaplingBlock oakSapling("assets/texture/block/oak_sapling.png", SaplingBlock::eTreeType_Oak);
    static SaplingBlock spruceSapling("assets/texture/block/spruce_sapling.png", SaplingBlock::eTreeType_Spruce);
    static SaplingBlock birchSapling("assets/texture/block/birch_sapling.png", SaplingBlock::eTreeType_Birch);

    static SaplingBlock acaciaSapling("assets/texture/block/acacia_sapling.png", SaplingBlock::eTreeType_Acacia);
    SaplingBlock *saplings[]   = {&oakSapling, &spruceSapling, &birchSapling, &acaciaSapling};
    const char *saplingNames[] = {"oak_sapling", "spruce_sapling", "birch_sapling", "acacia_sapling"};
    for (size_t i = 0; i < sizeof(saplings) / sizeof(saplings[0]); i++)
    {
        saplings[i]->SetBreakSound("dig.grass");
        registry->Register(saplingNames[i], saplings[i]);
    }

    static PricklyVineBlock pricklyVine("assets/texture/block/prickly_vine");
    pricklyVine.SetBreakSound("dig.grass");
    registry->Register("prickly_vine", &pricklyVine);
    registry->Register("clay", &clay);
    registry->Register("coarse_dirt", &coarseDirt);
    registry->Register("furnace", &furnace);
    registry->Register("lit_furnace", &litFurnace);
    static TntBlock tnt;
    registry->Register("tnt", &tnt);
    static FarmlandBlock farmland("assets/texture/block/dirt.png", "assets/texture/block/farmland_dry.png", "assets/texture/block/farmland_wet.png");
    farmland.SetBreakSound("dig.dirt");
    registry->Register("farmland", &farmland);
    static DirtPathBlock dirtPath("assets/texture/block/dirt_path_side.png", "assets/texture/block/dirt_path_top.png",
                                  "assets/texture/block/dirt.png");
    dirtPath.SetBreakSound("dig.grass");
    registry->Register("dirt_path", &dirtPath);

    static CropBlock wheat("assets/texture/block/wheat_stage", 8, 7, "wheat_seeds");
    registry->Register("wheat", &wheat);
    static CropBlock carrots("assets/texture/block/carrots_stage", 4, 7, "carrot");
    registry->Register("carrots", &carrots);
    static CropBlock potatoes("assets/texture/block/potatoes_stage", 4, 7, "potato");
    registry->Register("potatoes", &potatoes);
    static CropBlock beetroots("assets/texture/block/beetroots_stage", 4, 3, "beetroot_seeds");
    registry->Register("beetroots", &beetroots);

    static Block pumpkin(true, "assets/texture/block/pumpkin_side.png");
    pumpkin.SetTexturePath(Direction::up, "assets/texture/block/pumpkin_top.png");
    pumpkin.SetTexturePath(Direction::down, "assets/texture/block/pumpkin_top.png");
    pumpkin.SetBreakSound("dig.wood");
    registry->Register("pumpkin", &pumpkin);
    static FurnaceBlock carvedPumpkin("assets/texture/block/pumpkin_side.png", "assets/texture/block/pumpkin_top.png",
                                      "assets/texture/block/carved_pumpkin.png", false, false);
    carvedPumpkin.SetBreakSound("dig.wood");
    registry->Register("carved_pumpkin", &carvedPumpkin);
    static MelonBlock melon("assets/texture/block/melon_side.png", "assets/texture/block/melon_top.png");
    melon.SetBreakSound("dig.wood");
    registry->Register("melon", &melon);

    static StemBlock pumpkinStem("assets/texture/block/pumpkin_stem_stage", "pumpkin", "attached_pumpkin_stem");
    registry->Register("pumpkin_stem", &pumpkinStem);
    static AttachedStemBlock attachedPumpkinStem("assets/texture/block/attached_pumpkin_stem.png", "pumpkin", &pumpkinStem);
    registry->Register("attached_pumpkin_stem", &attachedPumpkinStem);
    static StemBlock melonStem("assets/texture/block/melon_stem_stage", "melon", "attached_melon_stem");
    registry->Register("melon_stem", &melonStem);
    static AttachedStemBlock attachedMelonStem("assets/texture/block/attached_melon_stem.png", "melon", &melonStem);
    registry->Register("attached_melon_stem", &attachedMelonStem);
    static Block coalOre(true, "assets/texture/block/coal_ore.png");
    registry->Register("coal_ore", &coalOre);
    static Block ironOre(true, "assets/texture/block/iron_ore.png");
    registry->Register("iron_ore", &ironOre);
    static Block goldOre(true, "assets/texture/block/gold_ore.png");
    registry->Register("gold_ore", &goldOre);
    static Block diamondOre(true, "assets/texture/block/diamond_ore.png");
    registry->Register("diamond_ore", &diamondOre);
    static Block emeraldOre(true, "assets/texture/block/emerald_ore.png");
    registry->Register("emerald_ore", &emeraldOre);
    static Block lapisOre(true, "assets/texture/block/lapis_ore.png");
    registry->Register("lapis_ore", &lapisOre);
    static Block coalBlock(true, "assets/texture/block/coal_block.png");
    registry->Register("coal_block", &coalBlock);
    static Block ironBlock(true, "assets/texture/block/iron_block.png");
    registry->Register("iron_block", &ironBlock);
    static Block goldBlock(true, "assets/texture/block/gold_block.png");
    registry->Register("gold_block", &goldBlock);
    static Block diamondBlock(true, "assets/texture/block/diamond_block.png");
    registry->Register("diamond_block", &diamondBlock);
    static Block emeraldBlock(true, "assets/texture/block/emerald_block.png");
    registry->Register("emerald_block", &emeraldBlock);
    static Block lapisBlock(true, "assets/texture/block/lapis_block.png");
    registry->Register("lapis_block", &lapisBlock);

    static ChestBlock chest;
    registry->Register("chest", &chest);
    static BedBlock redBed("red");
    registry->Register("red_bed", &redBed);
    static MobSpawnerBlock mobSpawner;
    registry->Register("mob_spawner", &mobSpawner);
    static LanternBlock orangeLantern("orange_lantern", 0xFF, 0xE0, 0xB0);
    registry->Register("orange_lantern", &orangeLantern);
    static ChainBlock chain;
    registry->Register("chain", &chain);
    static PaneBlock ironBars("assets/texture/block/iron_bars.png", "assets/texture/block/iron_bars.png");
    ironBars.SetBreakSound("dig.stone", 4);
    ironBars.SetPlaceSound("", 4, 0.8f);
    registry->Register("iron_bars", &ironBars);

    static SlabBlock birchSlab("assets/texture/block/birch_planks.png", &birchPlanks);
    birchSlab.SetBreakSound("dig.wood");
    registry->Register("birch_slab", &birchSlab);
    static StairBlock birchStairs("assets/texture/block/birch_planks.png");
    birchStairs.SetBreakSound("dig.wood");
    registry->Register("birch_stairs", &birchStairs);
    static SlabBlock acaciaSlab("assets/texture/block/acacia_planks.png", &acaciaPlanks);
    acaciaSlab.SetBreakSound("dig.wood");
    registry->Register("acacia_slab", &acaciaSlab);
    static StairBlock acaciaStairs("assets/texture/block/acacia_planks.png");
    acaciaStairs.SetBreakSound("dig.wood");
    registry->Register("acacia_stairs", &acaciaStairs);
    static SlabBlock cobblestoneSlab("assets/texture/block/cobblestone.png", &cobblestone);
    registry->Register("cobblestone_slab", &cobblestoneSlab);
    static StairBlock cobblestoneStairs("assets/texture/block/cobblestone.png");
    registry->Register("cobblestone_stairs", &cobblestoneStairs);
    static SlabBlock sandstoneSlab("assets/texture/block/sandstone.png", &sandstone);
    sandstoneSlab.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_top.png");
    sandstoneSlab.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_bottom.png");
    registry->Register("sandstone_slab", &sandstoneSlab);
    static StairBlock sandstoneStairs("assets/texture/block/sandstone.png");
    sandstoneStairs.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_top.png");
    sandstoneStairs.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_bottom.png");
    registry->Register("sandstone_stairs", &sandstoneStairs);
    static SlabBlock redSandstoneSlab("assets/texture/block/red_sandstone.png", &redSandstone);
    redSandstoneSlab.SetFaceTexture(Direction::up, "assets/texture/block/red_sandstone_top.png");
    redSandstoneSlab.SetFaceTexture(Direction::down, "assets/texture/block/red_sandstone_bottom.png");
    registry->Register("red_sandstone_slab", &redSandstoneSlab);
    static StairBlock redSandstoneStairs("assets/texture/block/red_sandstone.png");
    redSandstoneStairs.SetFaceTexture(Direction::up, "assets/texture/block/red_sandstone_top.png");
    redSandstoneStairs.SetFaceTexture(Direction::down, "assets/texture/block/red_sandstone_bottom.png");
    registry->Register("red_sandstone_stairs", &redSandstoneStairs);
    static Block terracotta(true, "assets/texture/block/terracotta.png");
    registry->Register("terracotta", &terracotta);
    static Block hayBlock(true, "assets/texture/block/hay_block_side.png");
    hayBlock.SetBreakSound("dig.grass");
    hayBlock.SetPillar("assets/texture/block/hay_block_top.png");
    registry->Register("hay_block", &hayBlock);

    static TrapDoorBlock oakTrapDoor("assets/texture/block/trapdoor.png");
    registry->Register("oak_trapdoor", &oakTrapDoor);

    static TrapDoorBlock spruceTrapDoor("assets/texture/block/spruce_trapdoor.png");
    registry->Register("spruce_trapdoor", &spruceTrapDoor);

    static TrapDoorBlock birchTrapDoor("assets/texture/block/birch_trapdoor.png");
    registry->Register("birch_trapdoor", &birchTrapDoor);

    static TrapDoorBlock acaciaTrapDoor("assets/texture/block/acacia_trapdoor.png");
    registry->Register("acacia_trapdoor", &acaciaTrapDoor);

    static WallBlock cobblestoneWall("assets/texture/block/cobblestone.png");
    registry->Register("cobblestone_wall", &cobblestoneWall);

    static WallBlock mossyCobblestoneWall("assets/texture/block/mossy_cobblestone.png");
    registry->Register("mossy_cobblestone_wall", &mossyCobblestoneWall);

    const char *colorNames[]   = {"white", "light_gray", "gray", "black",      "brown", "red",    "orange",  "yellow",
                                  "lime",  "green",      "cyan", "light_blue", "blue",  "purple", "magenta", "pink"};
    uint8_t lanternColors[][3] = {{0xFF, 0xFF, 0xFF}, {0xC8, 0xC8, 0xC8}, {0x90, 0x90, 0x98}, {0x60, 0x50, 0x80},
                                  {0xD0, 0x90, 0x58}, {0xFF, 0x40, 0x30}, {0xFF, 0x90, 0x30}, {0xFF, 0xE8, 0x40},
                                  {0x90, 0xFF, 0x40}, {0x40, 0xC0, 0x40}, {0x30, 0xE0, 0xE0}, {0x70, 0xC8, 0xFF},
                                  {0x40, 0x60, 0xFF}, {0xA0, 0x50, 0xFF}, {0xFF, 0x50, 0xE0}, {0xFF, 0x90, 0xC0}};
    static std::vector<std::unique_ptr<Block>> coloredBlocks;
    for (size_t i = 0; i < std::size(colorNames); i++)
    {
        std::string color = colorNames[i];
        if (color != "red")
        {
            std::unique_ptr<Block> bed = std::make_unique<BedBlock>(color);
            registry->Register(color + "_bed", bed.get());
            coloredBlocks.push_back(std::move(bed));
        }

        std::unique_ptr<Block> carpet = std::make_unique<CarpetBlock>("assets/texture/block/" + color + "_wool.png");
        registry->Register(color + "_carpet", carpet.get());
        coloredBlocks.push_back(std::move(carpet));

        if (color != "orange")
        {
            std::unique_ptr<Block> lantern =
                    std::make_unique<LanternBlock>(color + "_lantern", lanternColors[i][0], lanternColors[i][1], lanternColors[i][2]);
            registry->Register(color + "_lantern", lantern.get());
            coloredBlocks.push_back(std::move(lantern));
        }
    }

    static Block brimstone(true, "assets/texture/block/brimstone.png");
    registry->Register("brimstone", &brimstone);

    static LilyPadBlock lilyPad;
    registry->Register("lily_pad", &lilyPad);

    static Block jungleLog(true, "assets/texture/block/jungle_log.png");
    jungleLog.SetBreakSound("dig.wood");
    jungleLog.SetPillar("assets/texture/block/jungle_log_top.png");
    registry->Register("jungle_log", &jungleLog);

    static Block junglePlanks(true, "assets/texture/block/jungle_planks.png");
    junglePlanks.SetBreakSound("dig.wood");
    registry->Register("jungle_planks", &junglePlanks);

    static LeavesBlock jungleLeaves("assets/texture/block/leaves_jungle.png");
    jungleLeaves.SetFoliageTint();
    registry->Register("jungle_leaves", &jungleLeaves);

    static SaplingBlock jungleSapling("assets/texture/block/jungle_sapling.png", SaplingBlock::eTreeType_Jungle);
    jungleSapling.SetBreakSound("dig.grass");
    registry->Register("jungle_sapling", &jungleSapling);

    static DoorBlock jungleDoor("assets/texture/block/door_jungle_lower.png", "assets/texture/block/door_jungle_upper.png");
    jungleDoor.SetBreakSound("dig.wood");
    registry->Register("jungle_door", &jungleDoor);

    static FenceBlock jungleFence("assets/texture/block/jungle_planks.png");
    registry->Register("jungle_fence", &jungleFence);

    static FenceGateBlock jungleFenceGate("assets/texture/block/jungle_planks.png");
    registry->Register("jungle_fence_gate", &jungleFenceGate);

    static ButtonBlock jungleButton("assets/texture/block/jungle_planks.png", 30);
    jungleButton.SetBreakSound("dig.wood");
    registry->Register("jungle_button", &jungleButton);

    static PressurePlateBlock junglePressurePlate("assets/texture/block/jungle_planks.png");
    junglePressurePlate.SetBreakSound("dig.wood");
    registry->Register("jungle_pressure_plate", &junglePressurePlate);

    static SlabBlock jungleSlab("assets/texture/block/jungle_planks.png", &junglePlanks);
    jungleSlab.SetBreakSound("dig.wood");
    registry->Register("jungle_slab", &jungleSlab);

    static StairBlock jungleStairs("assets/texture/block/jungle_planks.png");
    jungleStairs.SetBreakSound("dig.wood");
    registry->Register("jungle_stairs", &jungleStairs);

    static TrapDoorBlock jungleTrapDoor("assets/texture/block/jungle_trapdoor.png");
    registry->Register("jungle_trapdoor", &jungleTrapDoor);

    static Block cherryLog(true, "assets/texture/block/cherry_log.png");
    cherryLog.SetBreakSound("dig.wood");
    cherryLog.SetPillar("assets/texture/block/cherry_log_top.png");
    registry->Register("cherry_log", &cherryLog);

    static Block cherryPlanks(true, "assets/texture/block/cherry_planks.png");
    cherryPlanks.SetBreakSound("dig.wood");
    registry->Register("cherry_planks", &cherryPlanks);

    static LeavesBlock cherryLeaves("assets/texture/block/cherry_leaves.png");
    registry->Register("cherry_leaves", &cherryLeaves);

    static SaplingBlock cherrySapling("assets/texture/block/cherry_sapling.png", SaplingBlock::eTreeType_Cherry);
    cherrySapling.SetBreakSound("dig.grass");
    registry->Register("cherry_sapling", &cherrySapling);

    static DoorBlock cherryDoor("assets/texture/block/cherry_door_bottom.png", "assets/texture/block/cherry_door_top.png");
    cherryDoor.SetBreakSound("dig.wood");
    registry->Register("cherry_door", &cherryDoor);

    static FenceBlock cherryFence("assets/texture/block/cherry_planks.png");
    registry->Register("cherry_fence", &cherryFence);

    static FenceGateBlock cherryFenceGate("assets/texture/block/cherry_planks.png");
    registry->Register("cherry_fence_gate", &cherryFenceGate);

    static ButtonBlock cherryButton("assets/texture/block/cherry_planks.png", 30);
    cherryButton.SetBreakSound("dig.wood");
    registry->Register("cherry_button", &cherryButton);

    static PressurePlateBlock cherryPressurePlate("assets/texture/block/cherry_planks.png");
    cherryPressurePlate.SetBreakSound("dig.wood");
    registry->Register("cherry_pressure_plate", &cherryPressurePlate);

    static SlabBlock cherrySlab("assets/texture/block/cherry_planks.png", &cherryPlanks);
    cherrySlab.SetBreakSound("dig.wood");
    registry->Register("cherry_slab", &cherrySlab);

    static StairBlock cherryStairs("assets/texture/block/cherry_planks.png");
    cherryStairs.SetBreakSound("dig.wood");
    registry->Register("cherry_stairs", &cherryStairs);

    static TrapDoorBlock cherryTrapDoor("assets/texture/block/cherry_trapdoor.png");
    registry->Register("cherry_trapdoor", &cherryTrapDoor);

    static PinkPetalsBlock pinkPetals("assets/texture/block/pink_petals.png");
    registry->Register("pink_petals", &pinkPetals);

    static Block bambooBlock(true, "assets/texture/block/bamboo_block.png");
    bambooBlock.SetBreakSound("dig.wood");
    bambooBlock.SetPillar("assets/texture/block/bamboo_block_top.png");
    registry->Register("bamboo_block", &bambooBlock);

    static Block bambooPlanks(true, "assets/texture/block/bamboo_planks.png");
    bambooPlanks.SetBreakSound("dig.wood");
    registry->Register("bamboo_planks", &bambooPlanks);

    static Block bambooMosaic(true, "assets/texture/block/bamboo_mosaic.png");
    bambooMosaic.SetBreakSound("dig.wood");
    registry->Register("bamboo_mosaic", &bambooMosaic);

    static SlabBlock bambooMosaicSlab("assets/texture/block/bamboo_mosaic.png", &bambooMosaic);
    bambooMosaicSlab.SetBreakSound("dig.wood");
    registry->Register("bamboo_mosaic_slab", &bambooMosaicSlab);

    static StairBlock bambooMosaicStairs("assets/texture/block/bamboo_mosaic.png");
    bambooMosaicStairs.SetBreakSound("dig.wood");
    registry->Register("bamboo_mosaic_stairs", &bambooMosaicStairs);

    static DoorBlock bambooDoor("assets/texture/block/bamboo_door_bottom.png", "assets/texture/block/bamboo_door_top.png");
    bambooDoor.SetBreakSound("dig.wood");
    registry->Register("bamboo_door", &bambooDoor);

    static FenceBlock bambooFence("assets/texture/block/bamboo_planks.png");
    registry->Register("bamboo_fence", &bambooFence);

    static FenceGateBlock bambooFenceGate("assets/texture/block/bamboo_planks.png");
    registry->Register("bamboo_fence_gate", &bambooFenceGate);

    static ButtonBlock bambooButton("assets/texture/block/bamboo_planks.png", 30);
    bambooButton.SetBreakSound("dig.wood");
    registry->Register("bamboo_button", &bambooButton);

    static PressurePlateBlock bambooPressurePlate("assets/texture/block/bamboo_planks.png");
    bambooPressurePlate.SetBreakSound("dig.wood");
    registry->Register("bamboo_pressure_plate", &bambooPressurePlate);

    static SlabBlock bambooSlab("assets/texture/block/bamboo_planks.png", &bambooPlanks);
    bambooSlab.SetBreakSound("dig.wood");
    registry->Register("bamboo_slab", &bambooSlab);

    static StairBlock bambooStairs("assets/texture/block/bamboo_planks.png");
    bambooStairs.SetBreakSound("dig.wood");
    registry->Register("bamboo_stairs", &bambooStairs);

    static TrapDoorBlock bambooTrapDoor("assets/texture/block/bamboo_trapdoor.png");
    registry->Register("bamboo_trapdoor", &bambooTrapDoor);

    static CocoaBlock cocoa;
    registry->Register("cocoa", &cocoa);

    static BambooBlock bamboo;
    registry->Register("bamboo", &bamboo);

    static BambooSaplingBlock bambooSapling;
    registry->Register("bamboo_sapling", &bambooSapling);

    static ScaffoldingBlock scaffolding;
    registry->Register("scaffolding", &scaffolding);

    static Block mossBlock(true, "assets/texture/block/moss_block.png");
    mossBlock.SetBreakSound("moss.break", 4);
    registry->Register("moss_block", &mossBlock);

    static CarpetBlock mossCarpet("assets/texture/block/moss_block.png");
    mossCarpet.SetBreakSound("moss.break", 4);
    registry->Register("moss_carpet", &mossCarpet);

    static SweetBerryBushBlock sweetBerryBush;
    registry->Register("sweet_berry_bush", &sweetBerryBush);
    registry->Register("podzol", &podzol);

    static Block sandstoneCoalOre(true, "assets/texture/block/sandstone_coal_ore.png");
    sandstoneCoalOre.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_coal_ore_top.png");
    sandstoneCoalOre.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_coal_ore_bottom.png");
    registry->Register("sandstone_coal_ore", &sandstoneCoalOre);
    static Block sandstoneIronOre(true, "assets/texture/block/sandstone_iron_ore.png");
    sandstoneIronOre.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_iron_ore_top.png");
    sandstoneIronOre.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_iron_ore_bottom.png");
    registry->Register("sandstone_iron_ore", &sandstoneIronOre);
    static Block sandstoneGoldOre(true, "assets/texture/block/sandstone_gold_ore.png");
    sandstoneGoldOre.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_gold_ore_top.png");
    sandstoneGoldOre.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_gold_ore_bottom.png");
    registry->Register("sandstone_gold_ore", &sandstoneGoldOre);
    static Block sandstoneDiamondOre(true, "assets/texture/block/sandstone_diamond_ore.png");
    sandstoneDiamondOre.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_diamond_ore_top.png");
    sandstoneDiamondOre.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_diamond_ore_bottom.png");
    registry->Register("sandstone_diamond_ore", &sandstoneDiamondOre);
    static Block sandstoneEmeraldOre(true, "assets/texture/block/sandstone_emerald_ore.png");
    sandstoneEmeraldOre.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_emerald_ore_top.png");
    sandstoneEmeraldOre.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_emerald_ore_bottom.png");
    registry->Register("sandstone_emerald_ore", &sandstoneEmeraldOre);
    static Block sandstoneLapisOre(true, "assets/texture/block/sandstone_lapis_ore.png");
    sandstoneLapisOre.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_lapis_ore_top.png");
    sandstoneLapisOre.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_lapis_ore_bottom.png");
    registry->Register("sandstone_lapis_ore", &sandstoneLapisOre);
    static Block iceCoalOre(true, "assets/texture/block/ice_coal_ore.png");
    iceCoalOre.SetBreakSound("random.glass", 3);
    iceCoalOre.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_coal_ore", &iceCoalOre);
    static Block iceIronOre(true, "assets/texture/block/ice_iron_ore.png");
    iceIronOre.SetBreakSound("random.glass", 3);
    iceIronOre.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_iron_ore", &iceIronOre);
    static Block iceGoldOre(true, "assets/texture/block/ice_gold_ore.png");
    iceGoldOre.SetBreakSound("random.glass", 3);
    iceGoldOre.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_gold_ore", &iceGoldOre);
    static Block iceDiamondOre(true, "assets/texture/block/ice_diamond_ore.png");
    iceDiamondOre.SetBreakSound("random.glass", 3);
    iceDiamondOre.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_diamond_ore", &iceDiamondOre);
    static Block iceEmeraldOre(true, "assets/texture/block/ice_emerald_ore.png");
    iceEmeraldOre.SetBreakSound("random.glass", 3);
    iceEmeraldOre.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_emerald_ore", &iceEmeraldOre);
    static Block iceLapisOre(true, "assets/texture/block/ice_lapis_ore.png");
    iceLapisOre.SetBreakSound("random.glass", 3);
    iceLapisOre.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_lapis_ore", &iceLapisOre);

    static BrittleIceBlock brittleIce;
    brittleIce.SetBreakSound("random.glass", 3);
    brittleIce.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("brittle_ice", &brittleIce);

    static Block darkOakLog(true, "assets/texture/block/dark_oak_log.png");
    darkOakLog.SetBreakSound("dig.wood");
    darkOakLog.SetPillar("assets/texture/block/dark_oak_log_top.png");
    registry->Register("dark_oak_log", &darkOakLog);

    static Block darkOakPlanks(true, "assets/texture/block/dark_oak_planks.png");
    darkOakPlanks.SetBreakSound("dig.wood");
    registry->Register("dark_oak_planks", &darkOakPlanks);

    static LeavesBlock darkOakLeaves("assets/texture/block/dark_oak_leaves.png");
    darkOakLeaves.SetFoliageTint();
    registry->Register("dark_oak_leaves", &darkOakLeaves);

    static SaplingBlock darkOakSapling("assets/texture/block/dark_oak_sapling.png", SaplingBlock::eTreeType_DarkOak);
    darkOakSapling.SetBreakSound("dig.grass");
    registry->Register("dark_oak_sapling", &darkOakSapling);

    static DoorBlock darkOakDoor("assets/texture/block/dark_oak_door_bottom.png", "assets/texture/block/dark_oak_door_top.png");
    darkOakDoor.SetBreakSound("dig.wood");
    registry->Register("dark_oak_door", &darkOakDoor);

    static FenceBlock darkOakFence("assets/texture/block/dark_oak_planks.png");
    registry->Register("dark_oak_fence", &darkOakFence);

    static FenceGateBlock darkOakFenceGate("assets/texture/block/dark_oak_planks.png");
    registry->Register("dark_oak_fence_gate", &darkOakFenceGate);

    static ButtonBlock darkOakButton("assets/texture/block/dark_oak_planks.png", 30);
    darkOakButton.SetBreakSound("dig.wood");
    registry->Register("dark_oak_button", &darkOakButton);

    static PressurePlateBlock darkOakPressurePlate("assets/texture/block/dark_oak_planks.png");
    darkOakPressurePlate.SetBreakSound("dig.wood");
    registry->Register("dark_oak_pressure_plate", &darkOakPressurePlate);

    static SlabBlock darkOakSlab("assets/texture/block/dark_oak_planks.png", &darkOakPlanks);
    darkOakSlab.SetBreakSound("dig.wood");
    registry->Register("dark_oak_slab", &darkOakSlab);

    static StairBlock darkOakStairs("assets/texture/block/dark_oak_planks.png");
    darkOakStairs.SetBreakSound("dig.wood");
    registry->Register("dark_oak_stairs", &darkOakStairs);

    static TrapDoorBlock darkOakTrapDoor("assets/texture/block/dark_oak_trapdoor.png");
    registry->Register("dark_oak_trapdoor", &darkOakTrapDoor);

    static Block basalt(true, "assets/texture/block/basalt_side.png");
    basalt.SetPillar("assets/texture/block/basalt_top.png");
    registry->Register("basalt", &basalt);

    const char *terracottaColors[] = {"white", "orange", "yellow", "light_gray", "brown", "red"};
    static std::vector<std::unique_ptr<Block>> coloredTerracotta;
    for (size_t i = 0; i < std::size(terracottaColors); i++)
    {
        std::string name = std::string(terracottaColors[i]) + "_terracotta";
        coloredTerracotta.push_back(std::make_unique<Block>(true, "assets/texture/block/" + name + ".png"));
        registry->Register(name, coloredTerracotta.back().get());
    }

    static EffectsExperimentBlock effectsExperiment;
    registry->Register("effects_experiment", &effectsExperiment);
    static CataclysmExperimentBlock singularityExperiment("singularity", eExplosionType_SingularityCharge, eExplosionType_SingularityBlast, 120,
                                                          16.0f);
    registry->Register("singularity_experiment", &singularityExperiment);
    static CataclysmExperimentBlock novaExperiment("nova", eExplosionType_NovaCharge, eExplosionType_NovaBlast, 120, 16.0f);
    registry->Register("nova_experiment", &novaExperiment);
    static CataclysmExperimentBlock tempestExperiment("tempest", eExplosionType_TempestCharge, eExplosionType_TempestBlast, 120, 16.0f);
    registry->Register("tempest_experiment", &tempestExperiment);
    static CataclysmExperimentBlock prismExperiment("prism", eExplosionType_PrismCharge, eExplosionType_PrismBlast, 120, 16.0f);
    registry->Register("prism_experiment", &prismExperiment);
    static CataclysmExperimentBlock hellgateExperiment("hellgate", eExplosionType_HellgateCharge, eExplosionType_HellgateBlast, 300, 22.0f);
    registry->Register("hellgate_experiment", &hellgateExperiment);

    const char *signWoods[]      = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak", "cherry", "bamboo"};
    const char *signPlankPaths[] = {"planks_oak",    "planks_spruce",   "birch_planks",  "jungle_planks",
                                    "acacia_planks", "dark_oak_planks", "cherry_planks", "bamboo_planks"};
    static std::vector<std::unique_ptr<SignBlock>> signs;
    for (size_t i = 0; i < std::size(signWoods); i++)
    {
        std::string plankPath  = std::string("assets/texture/block/") + signPlankPaths[i] + ".png";
        std::string entityPath = std::string("assets/texture/entity/sign/") + signWoods[i] + ".png";
        signs.push_back(std::make_unique<SignBlock>(plankPath, entityPath, nullptr));
        SignBlock *standing = signs.back().get();
        signs.push_back(std::make_unique<SignBlock>(plankPath, entityPath, standing));
        standing->SetWallVariant(signs.back().get());
        registry->Register(std::string(signWoods[i]) + "_sign", standing);
        registry->Register(std::string(signWoods[i]) + "_wall_sign", signs.back().get());
    }

    const char *basaltOreNames[] = {"coal", "iron", "gold", "diamond", "emerald", "lapis", "quartz"};
    static std::vector<std::unique_ptr<Block>> basaltOres;
    for (size_t i = 0; i < std::size(basaltOreNames); i++)
    {
        std::string name = std::string("basalt_") + basaltOreNames[i] + "_ore";
        basaltOres.push_back(std::make_unique<Block>(true, "assets/texture/block/" + name + ".png"));
        basaltOres.back()->SetFaceTexture(Direction::up, "assets/texture/block/" + name + "_top.png");
        basaltOres.back()->SetFaceTexture(Direction::down, "assets/texture/block/" + name + "_top.png");
        registry->Register(name, basaltOres.back().get());
    }

    const char *lateTerracottaColors[] = {"magenta", "light_blue", "lime", "pink", "gray", "cyan", "purple", "blue", "green", "black"};
    for (size_t i = 0; i < std::size(lateTerracottaColors); i++)
    {
        std::string name = std::string(lateTerracottaColors[i]) + "_terracotta";
        coloredTerracotta.push_back(std::make_unique<Block>(true, "assets/texture/block/" + name + ".png"));
        registry->Register(name, coloredTerracotta.back().get());
    }

    static Block bookshelf(true, "assets/texture/block/bookshelf.png");
    bookshelf.SetFaceTexture(Direction::up, "assets/texture/block/planks_oak.png");
    bookshelf.SetFaceTexture(Direction::down, "assets/texture/block/planks_oak.png");
    bookshelf.SetBreakSound("dig.wood");
    registry->Register("bookshelf", &bookshelf);

    static Block smoothStone(true, "assets/texture/block/smooth_stone.png");
    registry->Register("smooth_stone", &smoothStone);

    static SlabBlock smoothStoneSlab("assets/texture/block/smooth_stone_slab_side.png", &smoothStone);
    smoothStoneSlab.SetFaceTexture(Direction::up, "assets/texture/block/smooth_stone.png");
    smoothStoneSlab.SetFaceTexture(Direction::down, "assets/texture/block/smooth_stone.png");
    registry->Register("smooth_stone_slab", &smoothStoneSlab);
    static StairBlock smoothStoneStairs("assets/texture/block/smooth_stone.png");
    registry->Register("smooth_stone_stairs", &smoothStoneStairs);

    static Block redstoneOre(true, "assets/texture/block/redstone_ore.png");
    registry->Register("redstone_ore", &redstoneOre);
    static PoweredBlock redstoneBlock("assets/texture/block/redstone_block.png");
    registry->Register("redstone_block", &redstoneBlock);
    static RedstoneWireBlock redstoneWire;
    registry->Register("redstone_wire", &redstoneWire);
    static Block sandstoneRedstoneOre(true, "assets/texture/block/sandstone_redstone_ore.png");
    sandstoneRedstoneOre.SetFaceTexture(Direction::up, "assets/texture/block/sandstone_redstone_ore_top.png");
    sandstoneRedstoneOre.SetFaceTexture(Direction::down, "assets/texture/block/sandstone_redstone_ore_bottom.png");
    registry->Register("sandstone_redstone_ore", &sandstoneRedstoneOre);
    static Block iceRedstoneOre(true, "assets/texture/block/ice_redstone_ore.png");
    iceRedstoneOre.SetBreakSound("random.glass", 3);
    iceRedstoneOre.SetPlaceSound("step.stone", 6, 0.8f);
    registry->Register("ice_redstone_ore", &iceRedstoneOre);
    static Block basaltRedstoneOre(true, "assets/texture/block/basalt_redstone_ore.png");
    basaltRedstoneOre.SetFaceTexture(Direction::up, "assets/texture/block/basalt_redstone_ore_top.png");
    basaltRedstoneOre.SetFaceTexture(Direction::down, "assets/texture/block/basalt_redstone_ore_top.png");
    registry->Register("basalt_redstone_ore", &basaltRedstoneOre);

    static RedstoneTorchBlock redstoneTorch(true, false);
    static RedstoneTorchBlock redstoneTorchWall(true, true);
    static RedstoneTorchBlock unlitRedstoneTorch(false, false);
    static RedstoneTorchBlock unlitRedstoneTorchWall(false, true);
    redstoneTorch.SetWallVariant(&redstoneTorchWall);
    redstoneTorch.SetToggled(&unlitRedstoneTorch);
    redstoneTorchWall.SetToggled(&unlitRedstoneTorchWall);
    unlitRedstoneTorch.SetToggled(&redstoneTorch);
    unlitRedstoneTorchWall.SetToggled(&redstoneTorchWall);
    registry->Register("redstone_torch", &redstoneTorch);
    registry->Register("redstone_torch_wall", &redstoneTorchWall);
    registry->Register("unlit_redstone_torch", &unlitRedstoneTorch);
    registry->Register("unlit_redstone_torch_wall", &unlitRedstoneTorchWall);
    static RedstoneLampBlock redstoneLamp(false);
    static RedstoneLampBlock litRedstoneLamp(true);
    redstoneLamp.SetToggled(&litRedstoneLamp);
    litRedstoneLamp.SetToggled(&redstoneLamp);
    registry->Register("redstone_lamp", &redstoneLamp);
    registry->Register("lit_redstone_lamp", &litRedstoneLamp);
    static NoteBlock noteBlock;
    registry->Register("note_block", &noteBlock);
    static TargetBlock target;
    registry->Register("target", &target);
    static WeightedPressurePlateBlock lightWeightedPressurePlate("assets/texture/block/gold_block.png", 15);
    registry->Register("light_weighted_pressure_plate", &lightWeightedPressurePlate);
    static WeightedPressurePlateBlock heavyWeightedPressurePlate("assets/texture/block/iron_block.png", 150);
    registry->Register("heavy_weighted_pressure_plate", &heavyWeightedPressurePlate);
    static RepeaterBlock repeater;
    registry->Register("repeater", &repeater);
    static ComparatorBlock comparator;
    registry->Register("comparator", &comparator);
    static HopperBlock hopper;
    registry->Register("hopper", &hopper);
    static PistonBaseBlock piston(false);
    static PistonBaseBlock stickyPiston(true);
    static PistonHeadBlock pistonHead;
    static MovingPistonBlock movingPiston;
    registry->Register("piston", &piston);
    registry->Register("sticky_piston", &stickyPiston);
    registry->Register("piston_head", &pistonHead);
    registry->Register("moving_piston", &movingPiston);
    static ObserverBlock observer;
    registry->Register("observer", &observer);
    static DispenserBlock dispenser("assets/texture/block/dispenser_front.png", "assets/texture/block/dispenser_front_vertical.png");
    registry->Register("dispenser", &dispenser);
    static DropperBlock dropper;
    registry->Register("dropper", &dropper);

    static SlimeBlock slimeBlock;
    registry->Register("slime_block", &slimeBlock);

    static RailBlock rail("assets/texture/block/rail_normal.png", "assets/texture/block/rail_normal_turned.png", true);
    rail.SetBreakSound("dig.stone", 4);
    registry->Register("rail", &rail);

    static PoweredRailBlock poweredRail("assets/texture/block/rail_golden.png", "assets/texture/block/rail_golden_powered.png");
    poweredRail.SetBreakSound("dig.stone", 4);
    registry->Register("powered_rail", &poweredRail);

    static DetectorRailBlock detectorRail("assets/texture/block/rail_detector.png", "assets/texture/block/rail_detector_powered.png");
    detectorRail.SetBreakSound("dig.stone", 4);
    registry->Register("detector_rail", &detectorRail);
    registry->Register("sand_layer", &sandLayer);

    const char *logNames[] = {"oak_log", "spruce_log", "birch_log", "acacia_log", "jungle_log", "cherry_log", "dark_oak_log", "bamboo_block"};
    for (size_t i = 0; i < sizeof(logNames) / sizeof(logNames[0]); i++)
    {
        Block::ByName(logNames[i])->SetFlammable(5, 5);
    }

    const char *woodNames[] = {"oak_planks",
                               "spruce_planks",
                               "birch_planks",
                               "acacia_planks",
                               "oak_slab",
                               "oak_stairs",
                               "spruce_slab",
                               "spruce_stairs",
                               "birch_slab",
                               "birch_stairs",
                               "acacia_slab",
                               "acacia_stairs",
                               "crafting_table",
                               "oak_door",
                               "spruce_door",
                               "birch_door",
                               "acacia_door",
                               "oak_fence",
                               "oak_fence_gate",
                               "spruce_fence",
                               "spruce_fence_gate",
                               "birch_fence",
                               "birch_fence_gate",
                               "acacia_fence",
                               "acacia_fence_gate",
                               "oak_trapdoor",
                               "spruce_trapdoor",
                               "birch_trapdoor",
                               "acacia_trapdoor",
                               "jungle_planks",
                               "jungle_slab",
                               "jungle_stairs",
                               "jungle_door",
                               "jungle_fence",
                               "jungle_fence_gate",
                               "jungle_trapdoor",
                               "cherry_planks",
                               "cherry_slab",
                               "cherry_stairs",
                               "cherry_door",
                               "cherry_fence",
                               "cherry_fence_gate",
                               "cherry_trapdoor",
                               "bamboo_planks",
                               "bamboo_slab",
                               "bamboo_stairs",
                               "bamboo_door",
                               "bamboo_fence",
                               "bamboo_fence_gate",
                               "bamboo_trapdoor",
                               "bamboo_mosaic",
                               "bamboo_mosaic_slab",
                               "bamboo_mosaic_stairs",
                               "dark_oak_planks",
                               "dark_oak_slab",
                               "dark_oak_stairs",
                               "dark_oak_door",
                               "dark_oak_fence",
                               "dark_oak_fence_gate",
                               "dark_oak_trapdoor"};
    for (size_t i = 0; i < sizeof(woodNames) / sizeof(woodNames[0]); i++)
    {
        Block::ByName(woodNames[i])->SetFlammable(5, 20);
    }

    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block            = registry->ById(i);
        const std::string &name = registry->GetKey(i).GetPath();
        if (name.ends_with("_leaves") || name.find("_wool") != std::string::npos)
        {
            block->SetFlammable(30, 60);
        }
    }

    Block::ByName("grass")->SetFlammable(60, 100);
    Block::ByName("fern")->SetFlammable(60, 100);
    Block::ByName("vine")->SetFlammable(15, 100);
    Block::ByName("bamboo")->SetFlammable(60, 60);
    Block::ByName("scaffolding")->SetFlammable(60, 60);
    Block::ByName("bookshelf")->SetFlammable(30, 20);

    const std::unordered_map<std::string, float> soundResistances = {{"dig.wood", 3.0f},     {"dig.grass", 0.6f},      {"dig.dirt", 0.5f},
                                                                     {"dig.sand", 0.5f},     {"dig.snow", 0.2f},       {"dig.cloth", 0.8f},
                                                                     {"random.glass", 0.3f}, {"dripstone.break", 1.0f}};
    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block            = registry->ById(i);
        const std::string &name = registry->GetKey(i).GetPath();
        auto found              = soundResistances.find(block->GetBreakSound());
        if (block->IsLiquid())
        {
            block->SetExplosionResistance(100.0f);
        }
        else if (!block->IsCollidable() || name == "tnt")
        {
            block->SetExplosionResistance(0.0f);
        }
        else if (name.ends_with("_leaves"))
        {
            block->SetExplosionResistance(0.2f);
        }
        else if (name.find("sandstone") != std::string::npos)
        {
            block->SetExplosionResistance(0.8f);
        }
        else if (found != soundResistances.end())
        {
            block->SetExplosionResistance(found->second);
        }
    }

    Block::ByName("moving_piston")->SetExplosionResistance(0.0f);
    Block::ByName("ice")->SetExplosionResistance(0.5f);
    Block::ByName("bookshelf")->SetExplosionResistance(1.5f);
    Block::ByName("packed_ice")->SetExplosionResistance(0.5f);
    Block::ByName("magma")->SetExplosionResistance(0.5f);
    Block::ByName("obsidian")->SetExplosionResistance(1200.0f);
    Block::ByName("bedrock")->SetExplosionResistance(3600000.0f);
    Block::ByName("gravel")->SetExplosionResistance(0.6f);
    Block::ByName("clay")->SetExplosionResistance(0.6f);
    Block::ByName("terracotta")->SetExplosionResistance(4.2f);
    Block::ByName("cactus")->SetExplosionResistance(0.4f);
    Block::ByName("ladder")->SetExplosionResistance(0.4f);
    Block::ByName("crafting_table")->SetExplosionResistance(2.5f);
    Block::ByName("chest")->SetExplosionResistance(2.5f);
    Block::ByName("hopper")->SetExplosionResistance(4.8f);
    Block::ByName("furnace")->SetExplosionResistance(3.5f);
    Block::ByName("dispenser")->SetExplosionResistance(3.5f);
    Block::ByName("dropper")->SetExplosionResistance(3.5f);
    Block::ByName("chain")->SetExplosionResistance(6.0f);
    Block::ByName("iron_bars")->SetExplosionResistance(6.0f);
    Block::ByName("lit_furnace")->SetExplosionResistance(3.5f);
    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        if (registry->GetKey(i).GetPath().ends_with("_log"))
        {
            registry->ById(i)->SetExplosionResistance(2.0f);
        }
    }

    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block             = registry->ById(i);
        const std::string &sound = block->GetBreakSound();
        if (block->IsLiquid())
        {
            block->SetDestroyTime(100.0f);
        }
        else if (!block->IsCollidable())
        {
            block->SetDestroyTime(0.0f);
        }
        else if (sound == "dig.stone")
        {
            block->SetDestroyTime(2.0f);
            block->SetTool(Block::eToolType_Pickaxe, true, 0);
        }
        else if (sound == "dripstone.break")
        {
            block->SetDestroyTime(1.5f);
            block->SetTool(Block::eToolType_Pickaxe, true, 0);
        }
        else if (sound == "dig.wood")
        {
            block->SetDestroyTime(2.0f);
            block->SetTool(Block::eToolType_Axe, false, 0);
        }
        else if (sound == "dig.grass")
        {
            block->SetDestroyTime(0.6f);
            block->SetTool(Block::eToolType_Shovel, false, 0);
        }
        else if (sound == "dig.dirt" || sound == "dig.sand")
        {
            block->SetDestroyTime(0.5f);
            block->SetTool(Block::eToolType_Shovel, false, 0);
        }
        else if (sound == "dig.snow")
        {
            block->SetDestroyTime(0.2f);
            block->SetTool(Block::eToolType_Shovel, true, 0);
        }
        else if (sound == "dig.cloth")
        {
            block->SetDestroyTime(0.8f);
        }
        else if (sound == "random.glass")
        {
            block->SetDestroyTime(0.3f);
        }
    }

    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block            = registry->ById(i);
        const std::string &name = registry->GetKey(i).GetPath();
        if (name.ends_with("_leaves"))
        {
            block->SetDestroyTime(0.2f);
            block->SetTool(Block::eToolType_None, false, 0);
        }
        else if (name.find("sandstone") != std::string::npos)
        {
            block->SetDestroyTime(0.8f);
        }
        else if (name.find("stone_brick") != std::string::npos)
        {
            block->SetDestroyTime(1.5f);
        }
        else if (name.ends_with("_button") || name.ends_with("_pressure_plate"))
        {
            block->SetDestroyTime(0.5f);
        }
        else if (name.ends_with("_sign"))
        {
            block->SetDestroyTime(1.0f);
            block->SetExplosionResistance(1.0f);
            block->SetTool(Block::eToolType_Axe, false, 0);
        }
        else if (name.ends_with("_door") || name.ends_with("_trapdoor"))
        {
            block->SetDestroyTime(3.0f);
        }
    }

    const char *rockNames[] = {"stone",           "andesite", "polished_andesite", "granite", "diorite", "polished_granite", "polished_diorite",
                               "checkered_tiles", "brimstone"};
    for (size_t i = 0; i < sizeof(rockNames) / sizeof(rockNames[0]); i++)
    {
        Block::ByName(rockNames[i])->SetDestroyTime(1.5f);
    }

    const char *railNames[] = {"rail", "powered_rail", "detector_rail"};
    for (size_t i = 0; i < sizeof(railNames) / sizeof(railNames[0]); i++)
    {
        Block::ByName(railNames[i])->SetDestroyTime(0.7f);
        Block::ByName(railNames[i])->SetTool(Block::eToolType_Pickaxe, false, 0);
    }

    Block::ByName("bedrock")->SetDestroyTime(-1.0f);
    Block::ByName("obsidian")->SetDestroyTime(50.0f);
    const char *fractureFrameNames[] = {"sealed_fracture_frame", "cracked_fracture_frame", "fractured_fracture_frame", "broken_fracture_frame",
                                        "resonant_socket"};
    for (const char *name : fractureFrameNames)
    {
        Block::ByName(name)->SetDestroyTime(-1.0f);
        Block::ByName(name)->SetExplosionResistance(3600000.0f);
    }

    Block::ByName("obsidian")->SetTool(Block::eToolType_Pickaxe, true, 3);
    Block::ByName("resonant_crystal")->SetDestroyTime(30.0f);
    Block::ByName("resonant_crystal")->SetExplosionResistance(1200.0f);
    Block::ByName("resonant_crystal")->SetTool(Block::eToolType_Pickaxe, true, 2);
    Block::ByName("fracture_core")->SetDestroyTime(50.0f);
    Block::ByName("fracture_core")->SetExplosionResistance(1200.0f);
    Block::ByName("fracture_core")->SetTool(Block::eToolType_Pickaxe, true, 3);
    Block::ByName("echo_quartz")->SetDestroyTime(1.5f);
    Block::ByName("bookshelf")->SetDestroyTime(1.5f);
    Block::ByName("echo_quartz")->SetExplosionResistance(6.0f);
    Block::ByName("echo_quartz")->SetTool(Block::eToolType_Pickaxe, true, 1);
    Block::ByName("polished_echo_quartz")->SetDestroyTime(1.5f);
    Block::ByName("polished_echo_quartz")->SetExplosionResistance(6.0f);
    Block::ByName("polished_echo_quartz")->SetTool(Block::eToolType_Pickaxe, true, 1);
    Block::ByName("echo_quartz_bricks")->SetDestroyTime(1.5f);
    Block::ByName("echo_quartz_bricks")->SetExplosionResistance(6.0f);
    Block::ByName("echo_quartz_bricks")->SetTool(Block::eToolType_Pickaxe, true, 1);
    Block::ByName("echo_quartz_pillar")->SetDestroyTime(1.5f);
    Block::ByName("echo_quartz_pillar")->SetExplosionResistance(6.0f);
    Block::ByName("echo_quartz_pillar")->SetTool(Block::eToolType_Pickaxe, true, 1);
    Block::ByName("echo_glass")->SetDestroyTime(0.5f);
    Block::ByName("echo_glass")->SetExplosionResistance(0.5f);
    Block::ByName("hollow_core")->SetDestroyTime(30.0f);
    Block::ByName("hollow_core")->SetExplosionResistance(1200.0f);
    Block::ByName("hollow_core")->SetTool(Block::eToolType_Pickaxe, true, 3);
    Block::ByName("resonator")->SetDestroyTime(5.0f);
    Block::ByName("resonator")->SetExplosionResistance(1200.0f);
    Block::ByName("resonator")->SetTool(Block::eToolType_Pickaxe, true, 2);
    const char *anvilNames[] = {"anvil", "chipped_anvil", "damaged_anvil"};
    for (size_t i = 0; i < 3; i++)
    {
        Block::ByName(anvilNames[i])->SetDestroyTime(5.0f);
        Block::ByName(anvilNames[i])->SetExplosionResistance(1200.0f);
        Block::ByName(anvilNames[i])->SetTool(Block::eToolType_Pickaxe, true, 0);
    }

    Block::ByName("rich_resonant_crystal")->SetDestroyTime(30.0f);
    Block::ByName("rich_resonant_crystal")->SetExplosionResistance(1200.0f);
    Block::ByName("rich_resonant_crystal")->SetTool(Block::eToolType_Pickaxe, true, 2);
    const char *oreNames[] = {"coal_ore",
                              "iron_ore",
                              "gold_ore",
                              "diamond_ore",
                              "emerald_ore",
                              "lapis_ore",
                              "sandstone_coal_ore",
                              "sandstone_iron_ore",
                              "sandstone_gold_ore",
                              "sandstone_diamond_ore",
                              "sandstone_emerald_ore",
                              "sandstone_lapis_ore",
                              "ice_coal_ore",
                              "ice_iron_ore",
                              "ice_gold_ore",
                              "ice_diamond_ore",
                              "ice_emerald_ore",
                              "ice_lapis_ore",
                              "basalt_coal_ore",
                              "basalt_iron_ore",
                              "basalt_gold_ore",
                              "basalt_diamond_ore",
                              "basalt_emerald_ore",
                              "basalt_lapis_ore",
                              "redstone_ore",
                              "sandstone_redstone_ore",
                              "ice_redstone_ore",
                              "basalt_redstone_ore",
                              "basalt_quartz_ore"};
    const int oreLevels[]  = {0, 1, 2, 2, 2, 1, 0, 1, 2, 2, 2, 1, 0, 1, 2, 2, 2, 1, 0, 1, 2, 2, 2, 1, 2, 2, 2, 2, 0};
    for (size_t i = 0; i < sizeof(oreNames) / sizeof(oreNames[0]); i++)
    {
        Block::ByName(oreNames[i])->SetDestroyTime(3.0f);
        Block::ByName(oreNames[i])->SetExplosionResistance(3.0f);
        Block::ByName(oreNames[i])->SetTool(Block::eToolType_Pickaxe, true, oreLevels[i]);
    }

    const char *storageNames[] = {"coal_block", "iron_block", "gold_block", "diamond_block", "emerald_block", "lapis_block", "redstone_block"};
    const float storageTimes[] = {5.0f, 5.0f, 3.0f, 5.0f, 5.0f, 3.0f, 5.0f};
    const int storageLevels[]  = {0, 1, 2, 2, 2, 1, 0};
    for (size_t i = 0; i < sizeof(storageNames) / sizeof(storageNames[0]); i++)
    {
        Block::ByName(storageNames[i])->SetDestroyTime(storageTimes[i]);
        Block::ByName(storageNames[i])->SetExplosionResistance(10.0f);
        Block::ByName(storageNames[i])->SetTool(Block::eToolType_Pickaxe, true, storageLevels[i]);
    }

    Block::ByName("furnace")->SetDestroyTime(3.5f);
    Block::ByName("lit_furnace")->SetDestroyTime(3.5f);
    Block::ByName("crafting_table")->SetDestroyTime(2.5f);
    Block::ByName("chest")->SetDestroyTime(2.5f);
    Block::ByName("hopper")->SetDestroyTime(3.0f);
    Block::ByName("redstone_lamp")->SetDestroyTime(0.3f);
    Block::ByName("lit_redstone_lamp")->SetDestroyTime(0.3f);
    Block::ByName("note_block")->SetDestroyTime(0.8f);
    Block::ByName("target")->SetDestroyTime(0.5f);
    Block::ByName("light_weighted_pressure_plate")->SetDestroyTime(0.5f);
    Block::ByName("heavy_weighted_pressure_plate")->SetDestroyTime(0.5f);
    Block::ByName("repeater")->SetDestroyTime(0.0f);
    Block::ByName("comparator")->SetDestroyTime(0.0f);
    Block::ByName("observer")->SetDestroyTime(3.0f);
    Block::ByName("observer")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("hopper")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("dispenser")->SetDestroyTime(3.5f);
    Block::ByName("dispenser")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("dropper")->SetDestroyTime(3.5f);
    Block::ByName("dropper")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("piston")->SetDestroyTime(0.5f);
    Block::ByName("sticky_piston")->SetDestroyTime(0.5f);
    Block::ByName("piston_head")->SetDestroyTime(0.5f);
    Block::ByName("moving_piston")->SetDestroyTime(-1.0f);
    for (size_t i = 0; i < std::size(colorNames); i++)
    {
        Block::ByName(std::string(colorNames[i]) + "_bed")->SetDestroyTime(0.2f);
        Block::ByName(std::string(colorNames[i]) + "_carpet")->SetDestroyTime(0.1f);
        Block::ByName(std::string(colorNames[i]) + "_lantern")->SetDestroyTime(3.5f);
        Block::ByName(std::string(colorNames[i]) + "_lantern")->SetExplosionResistance(3.5f);
        Block::ByName(std::string(colorNames[i]) + "_lantern")->SetTool(Block::eToolType_Pickaxe, true, 0);
    }

    Block::ByName("chain")->SetDestroyTime(5.0f);
    Block::ByName("chain")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("iron_bars")->SetDestroyTime(5.0f);
    Block::ByName("iron_bars")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("mob_spawner")->SetDestroyTime(5.0f);
    Block::ByName("mob_spawner")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("magma")->SetDestroyTime(0.5f);
    Block::ByName("hay_block")->SetDestroyTime(0.5f);
    Block::ByName("grass_block")->SetDestroyTime(0.6f);
    Block::ByName("moss_block")->SetDestroyTime(0.1f);
    Block::ByName("moss_carpet")->SetDestroyTime(0.1f);
    Block::ByName("farmland")->SetDestroyTime(0.6f);
    Block::ByName("dirt_path")->SetDestroyTime(0.65f);
    Block::ByName("pumpkin")->SetDestroyTime(1.0f);
    Block::ByName("carved_pumpkin")->SetDestroyTime(1.0f);
    Block::ByName("melon")->SetDestroyTime(1.0f);
    Block::ByName("gravel")->SetDestroyTime(0.6f);
    Block::ByName("clay")->SetDestroyTime(0.6f);
    Block::ByName("terracotta")->SetDestroyTime(1.25f);
    Block::ByName("basalt")->SetDestroyTime(1.25f);
    Block::ByName("basalt")->SetExplosionResistance(4.2f);
    for (size_t i = 0; i < std::size(terracottaColors); i++)
    {
        Block::ByName(std::string(terracottaColors[i]) + "_terracotta")->SetDestroyTime(1.25f);
        Block::ByName(std::string(terracottaColors[i]) + "_terracotta")->SetExplosionResistance(4.2f);
    }

    for (size_t i = 0; i < std::size(lateTerracottaColors); i++)
    {
        Block::ByName(std::string(lateTerracottaColors[i]) + "_terracotta")->SetDestroyTime(1.25f);
        Block::ByName(std::string(lateTerracottaColors[i]) + "_terracotta")->SetExplosionResistance(4.2f);
    }

    Block::ByName("effects_experiment")->SetExplosionResistance(1200.0f);
    Block::ByName("singularity_experiment")->SetExplosionResistance(1200.0f);
    Block::ByName("nova_experiment")->SetExplosionResistance(1200.0f);
    Block::ByName("tempest_experiment")->SetExplosionResistance(1200.0f);
    Block::ByName("prism_experiment")->SetExplosionResistance(1200.0f);
    Block::ByName("hellgate_experiment")->SetExplosionResistance(1200.0f);

    Block::ByName("snow")->SetDestroyTime(0.1f);
    Block::ByName("snow")->SetTool(Block::eToolType_Shovel, true, 0);
    Block::ByName("sand_layer")->SetDestroyTime(0.1f);
    Block::ByName("sand_layer")->SetTool(Block::eToolType_Shovel, true, 0);
    Block::ByName("snow_block")->SetTool(Block::eToolType_Shovel, true, 0);
    Block::ByName("cactus")->SetDestroyTime(0.4f);
    Block::ByName("ladder")->SetDestroyTime(0.4f);
    Block::ByName("ladder")->SetTool(Block::eToolType_Axe, false, 0);
    Block::ByName("pointed_dripstone")->SetDestroyTime(1.5f);
    Block::ByName("pointed_dripstone")->SetTool(Block::eToolType_Pickaxe, true, 0);
    Block::ByName("vine")->SetDestroyTime(0.2f);
    Block::ByName("vine")->SetTool(Block::eToolType_None, false, 0);
    Block::ByName("prickly_vine")->SetDestroyTime(0.2f);
    Block::ByName("prickly_vine")->SetTool(Block::eToolType_None, false, 0);
    Block::ByName("tnt")->SetDestroyTime(0.0f);
    Block::ByName("sweet_berry_bush")->SetDestroyTime(0.0f);
    Block::ByName("sweet_berry_bush")->SetTool(Block::eToolType_None, false, 0);
    Block::ByName("sweet_berry_bush")->SetFlammable(60, 100);
    Block::ByName("bamboo")->SetDestroyTime(1.0f);
    Block::ByName("bamboo")->SetTool(Block::eToolType_Axe, false, 0);
    Block::ByName("bamboo_sapling")->SetDestroyTime(1.0f);
    Block::ByName("bamboo_sapling")->SetTool(Block::eToolType_Axe, false, 0);
    Block::ByName("scaffolding")->SetDestroyTime(0.0f);
    Block::ByName("scaffolding")->SetTool(Block::eToolType_None, false, 0);
    Block::ByName("cocoa")->SetDestroyTime(0.2f);
    Block::ByName("cocoa")->SetTool(Block::eToolType_Axe, false, 0);
    Block::ByName("tnt")->SetTool(Block::eToolType_None, false, 0);
    Block::ByName("stone_button")->SetTool(Block::eToolType_Pickaxe, false, 0);
    Block::ByName("stone_pressure_plate")->SetTool(Block::eToolType_Pickaxe, true, 0);
    const char *iceNames[] = {"ice", "packed_ice", "enchanted_ice", "icicle", "ice_sheet", "brittle_ice"};
    for (size_t i = 0; i < sizeof(iceNames) / sizeof(iceNames[0]); i++)
    {
        Block::ByName(iceNames[i])->SetDestroyTime(0.5f);
        Block::ByName(iceNames[i])->SetTool(Block::eToolType_Pickaxe, false, 0);
    }

    // We assign these after the loops above because destroy times and tools are derived from the break sound
    const char *basaltNames[] = {"basalt",           "basalt_coal_ore",     "basalt_iron_ore",
                                 "basalt_gold_ore",  "basalt_diamond_ore",  "basalt_emerald_ore",
                                 "basalt_lapis_ore", "basalt_redstone_ore", "basalt_quartz_ore"};
    for (size_t i = 0; i < std::size(basaltNames); i++)
    {
        Block::ByName(basaltNames[i])->SetBreakSound("basalt.break", 5);
    }

    Block::ByName("chain")->SetBreakSound("chain.break", 4);
    Block::ByName("cherry_leaves")->SetBreakSound("cherry_leaves.break", 5);
    Block::ByName("pink_petals")->SetBreakSound("cherry_leaves.break", 5);
    Block::ByName("pink_petals")->SetPlaceSound("cherry_leaves.break", 5, 0.96f);
    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block            = registry->ById(i);
        const std::string &name = registry->GetKey(i).GetPath();
        if (name.ends_with("_lantern"))
        {
            block->SetBreakSound("lantern.break", 6);
            block->SetPlaceSound("lantern.place", 6, 0.88f);
        }
        else if (name.starts_with("cherry_") && name != "cherry_leaves" && name != "cherry_sapling")
        {
            block->SetBreakSound("cherry_wood.break", 5);
        }
        else if (name.starts_with("bamboo_") && name != "bamboo_sapling")
        {
            block->SetBreakSound("bamboo_wood.break", 5);
        }
    }

    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block             = registry->ById(i);
        const std::string &sound = block->GetBreakSound();
        if (sound == "dig.stone" || sound == "dripstone.break" || sound == "basalt.break")
        {
            block->SetNoteInstrument("note.bd");
        }
        else if (sound == "dig.wood" || sound == "cherry_wood.break" || sound == "bamboo_wood.break")
        {
            block->SetNoteInstrument("note.bass");
        }
        else if (sound == "dig.sand")
        {
            block->SetNoteInstrument("note.snare");
        }
        else if (sound == "random.glass")
        {
            block->SetNoteInstrument("note.hat");
        }

        if (registry->GetKey(i).GetPath().ends_with("_wool"))
        {
            block->SetNoteInstrument("note.guitar");
        }
    }

    Block::ByName("gravel")->SetNoteInstrument("note.snare");
    Block::ByName("clay")->SetNoteInstrument("note.flute");
    Block::ByName("gold_block")->SetNoteInstrument("note.bell");
    Block::ByName("packed_ice")->SetNoteInstrument("note.icechime");
    Block::ByName("iron_block")->SetNoteInstrument("note.iron_xylophone");
    Block::ByName("pumpkin")->SetNoteInstrument("note.didgeridoo");
    Block::ByName("carved_pumpkin")->SetNoteInstrument("note.didgeridoo");
    Block::ByName("emerald_block")->SetNoteInstrument("note.bit");
    Block::ByName("hay_block")->SetNoteInstrument("note.banjo");
    Block::ByName("ice")->SetNoteInstrument("note.harp");
    Block::ByName("redstone_lamp")->SetNoteInstrument("note.harp");
    Block::ByName("lit_redstone_lamp")->SetNoteInstrument("note.harp");
}
