#include "BiomeMountains.h"

BiomeMountains::BiomeMountains()
{
    m_spawnsAnimals = false;
    m_topBlock      = "grass_block";
    m_fillerBlock   = "dirt";
    m_depth         = 0.45f;
    m_scale         = 0.7f;
    m_mountainDepth = 4.0f;
    m_grassColor    = 0x8AB689;
    m_foliageColor  = 0x6DA36B;
    m_snowLine      = 95;
    m_skyColor      = 0x7AA4FF;
    m_fogColor      = 0xC6D8F4;

    m_decorated         = true;
    m_treeDensity       = 1.0f;
    m_treeType          = eTreeType_SPRUCE;
    m_altTreeType       = eTreeType_OAK;
    m_altTreeChance     = 3;
    m_flowers           = {"dandelion", "dandelion", "poppy"};
    m_flowerPatches     = 1;
    m_flowerPatchChance = 4;
    m_flowerAttempts    = 8;
    m_emeralds          = true;
}
