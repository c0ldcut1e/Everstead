#include "BiomePlains.h"

BiomePlains::BiomePlains()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = 0.125f;
    m_scale        = 0.05f;
    m_grassColor   = 0x91BD59;
    m_foliageColor = 0x77AB2F;

    m_decorated         = true;
    m_treeDensity       = 1.0f / 20.0f;
    m_grassPatches      = 10;
    m_doublePlants      = {"tall_grass"};
    m_doublePlantMin    = 7;
    m_doublePlantMax    = 7;
    m_flowers           = {"dandelion",   "dandelion",   "dandelion",    "dandelion",   "poppy",     "poppy",
                           "azure_bluet", "oxeye_daisy", "orange_tulip", "white_tulip", "red_tulip", "pink_tulip"};
    m_flowerPatches     = 2;
    m_flowerPatchChance = 2;
    m_flowerAttempts    = 12;
}
