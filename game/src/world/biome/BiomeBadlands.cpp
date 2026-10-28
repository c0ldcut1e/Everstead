#include "BiomeBadlands.h"

BiomeBadlands::BiomeBadlands()
{
    m_spawnsAnimals  = false;
    m_spawnsChickens = false;
    m_topBlock       = "red_sand";
    m_fillerBlock    = "terracotta";
    m_depth          = 0.2f;
    m_scale          = 0.3f;
    m_grassColor     = 0x90814D;
    m_foliageColor   = 0x9E814D;
    m_hasRain        = false;
    m_skyColor       = 0x6EB1FF;
    m_fogColor       = 0xE0C9A8;

    m_desertPlants = true;
    m_dryBasins    = true;
    m_hotCaves     = true;
}
