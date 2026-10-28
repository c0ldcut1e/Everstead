#include "BiomeOcean.h"

BiomeOcean::BiomeOcean()
{
    m_spawnsAnimals  = false;
    m_spawnsChickens = false;
    m_topBlock       = "sand";
    m_fillerBlock    = "sand";
    m_depth          = -1.0f;
    m_scale          = 0.1f;
    m_grassColor     = 0x8EB971;
    m_foliageColor   = 0x71A74D;
    m_skyColor       = 0x78A2FF;
    m_fogColor       = 0xB8CFFF;

    m_continentalDepth = true;
    m_iceCaves         = false;
}
