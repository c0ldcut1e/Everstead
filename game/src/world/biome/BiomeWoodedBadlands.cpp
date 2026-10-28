#include "BiomeWoodedBadlands.h"

BiomeWoodedBadlands::BiomeWoodedBadlands()
{
    m_spawnsAnimals  = false;
    m_spawnsChickens = false;
    m_topBlock       = "grass_block";
    m_fillerBlock    = "terracotta";
    m_depth          = 1.5f;
    m_scale          = 0.025f;
    m_grassColor     = 0x90814D;
    m_foliageColor   = 0x9E814D;
    m_hasRain        = false;
    m_skyColor       = 0x6EB1FF;
    m_fogColor       = 0xE0C9A8;

    m_decorated   = true;
    m_treeDensity = 5.0f;
    m_dryBasins   = true;
    m_hotCaves    = true;
}
