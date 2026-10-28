#include "BiomeDesert.h"

BiomeDesert::BiomeDesert()
{
    m_spawnsAnimals  = false;
    m_spawnsChickens = false;
    m_topBlock       = "sand";
    m_fillerBlock    = "sand";
    m_depth          = 0.125f;
    m_scale          = 0.05f;
    m_grassColor     = 0xBFB755;
    m_foliageColor   = 0xAEA42A;
    m_hasRain        = false;
    m_skyColor       = 0x92AEE8;
    m_fogColor       = 0xD9D3BE;

    m_desertPlants      = true;
    m_sugarCaneAttempts = 50;
    m_dryBasins         = true;
    m_hotCaves          = true;
}
