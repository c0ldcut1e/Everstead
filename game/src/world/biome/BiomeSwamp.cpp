#include "BiomeSwamp.h"

BiomeSwamp::BiomeSwamp()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = -0.2f;
    m_scale        = 0.1f;
    m_grassColor   = 0x6A7039;
    m_foliageColor = 0x6A7039;
    m_skyColor     = 0x7BA4FF;
    m_fogColor     = 0xB8CCF2;

    m_decorated         = true;
    m_treeDensity       = 2.0f;
    m_treeType          = eTreeType_SWAMP;
    m_altTreeType       = eTreeType_SWAMP;
    m_grassPatches      = 5;
    m_flowers           = {"blue_orchid"};
    m_flowerPatches     = 1;
    m_flowerPatchChance = 1;
    m_flowerAttempts    = 12;
    m_sugarCaneAttempts = 10;
    m_lilyPads          = 4;
}
