#include "BiomeBirchForest.h"

BiomeBirchForest::BiomeBirchForest()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = 0.1f;
    m_scale        = 0.2f;
    m_grassColor   = 0x88BB67;
    m_foliageColor = 0x6BA941;
    m_skyColor     = 0x7CA6FC;
    m_fogColor     = 0xBDD2F4;

    m_decorated         = true;
    m_treeDensity       = 8.1f;
    m_treeType          = eTreeType_BIRCH;
    m_grassPatches      = 2;
    m_doublePlants      = {"lilac", "rose_bush", "peony"};
    m_doublePlantMin    = -3;
    m_doublePlantMax    = 1;
    m_flowers           = {"dandelion", "dandelion", "poppy"};
    m_flowerPatches     = 2;
    m_flowerPatchChance = 2;
    m_flowerAttempts    = 12;
}
