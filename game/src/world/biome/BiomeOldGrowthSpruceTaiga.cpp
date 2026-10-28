#include "BiomeOldGrowthSpruceTaiga.h"

BiomeOldGrowthSpruceTaiga::BiomeOldGrowthSpruceTaiga()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = 0.2f;
    m_scale        = 0.2f;
    m_grassColor   = 0x86B87F;
    m_foliageColor = 0x68A55F;
    m_skyColor     = 0x7AA2F6;
    m_fogColor     = 0xB6CBE6;

    m_decorated         = true;
    m_treeDensity       = 8.1f;
    m_treeType          = eTreeType_MEGA_SPRUCE;
    m_grassPatches      = 7;
    m_ferns             = true;
    m_doublePlants      = {"large_fern"};
    m_doublePlantMin    = 7;
    m_doublePlantMax    = 7;
    m_flowers           = {"dandelion", "dandelion", "poppy"};
    m_flowerPatches     = 1;
    m_flowerPatchChance = 4;
    m_flowerAttempts    = 8;
    m_podzolSurface     = true;
    m_forestRocks       = true;
}
