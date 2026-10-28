#include "BiomeJungleEdge.h"

BiomeJungleEdge::BiomeJungleEdge()
{
    m_grassColor       = 0x64C73F;
    m_foliageColor     = 0x3EB80F;
    m_skyColor         = 0x77A8FF;
    m_spawnsJungleMobs = true;

    m_decorated      = true;
    m_treeDensity    = 2.0f;
    m_treeType       = eTreeType_JUNGLE_EDGE;
    m_altTreeType    = eTreeType_JUNGLE_EDGE;
    m_grassPatches   = 25;
    m_ferns          = true;
    m_flowerPatches  = 4;
    m_flowerAttempts = 8;
    m_flowers        = {"dandelion", "poppy"};
    m_melons         = true;
    m_vineAttempts   = 50;
}
