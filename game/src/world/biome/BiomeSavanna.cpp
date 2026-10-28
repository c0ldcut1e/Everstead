#include "BiomeSavanna.h"

BiomeSavanna::BiomeSavanna()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = 0.125f;
    m_scale        = 0.05f;
    m_grassColor   = 0xBFB755;
    m_foliageColor = 0xAEA42A;
    m_hasRain      = false;
    m_skyColor     = 0x88AAF0;
    m_fogColor     = 0xD2D3C6;

    m_decorated         = true;
    m_treeDensity       = 1.1f;
    m_treeType          = eTreeType_ACACIA;
    m_grassPatches      = 20;
    m_flowers           = {"dandelion", "dandelion", "poppy"};
    m_flowerPatches     = 1;
    m_flowerPatchChance = 4;
    m_flowerAttempts    = 8;
    m_hotCaves          = true;
}
