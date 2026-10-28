#include "BiomeForest.h"

BiomeForest::BiomeForest()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = 0.1f;
    m_scale        = 0.2f;
    m_grassColor   = 0x79C05A;
    m_foliageColor = 0x59AE30;
    m_skyColor     = 0x7CA5FC;
    m_fogColor     = 0xBACFF4;

    m_decorated         = true;
    m_treeDensity       = 8.1f;
    m_altTreeType       = eTreeType_BIRCH;
    m_altTreeChance     = 5;
    m_grassPatches      = 2;
    m_doublePlants      = {"lilac", "rose_bush", "peony"};
    m_doublePlantMin    = -3;
    m_doublePlantMax    = 1;
    m_flowers           = {"dandelion", "dandelion", "poppy"};
    m_flowerPatches     = 2;
    m_flowerPatchChance = 2;
    m_flowerAttempts    = 12;
}
