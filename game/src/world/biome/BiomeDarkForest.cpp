#include "BiomeDarkForest.h"

BiomeDarkForest::BiomeDarkForest()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = 0.1f;
    m_scale        = 0.2f;
    m_grassColor   = 0x507A32;
    m_foliageColor = 0x59AE30;
    m_skyColor     = 0x7BA4FF;
    m_fogColor     = 0xA9BFDF;

    m_decorated     = true;
    m_treeDensity   = 12.0f;
    m_treeType      = eTreeType_DARK_OAK;
    m_altTreeType   = eTreeType_OAK;
    m_altTreeChance = 8;
    m_grassPatches  = 2;
}
