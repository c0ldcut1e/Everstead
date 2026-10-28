#include "BiomeBambooJungle.h"

BiomeBambooJungle::BiomeBambooJungle()
{
    m_grassColor   = 0x59C93C;
    m_foliageColor = 0x30BB0B;
    m_skyColor     = 0x77A8FF;

    m_decorated      = true;
    m_treeDensity    = 20.0f;
    m_treeType       = eTreeType_JUNGLE;
    m_altTreeType    = eTreeType_JUNGLE;
    m_grassPatches   = 25;
    m_ferns          = true;
    m_flowerPatches  = 4;
    m_flowerAttempts = 8;
    m_flowers        = {"dandelion", "poppy"};
    m_melons         = true;
    m_vineAttempts   = 50;
    m_bambooAttempts = 80;
    m_overgrownCaves = true;
}
