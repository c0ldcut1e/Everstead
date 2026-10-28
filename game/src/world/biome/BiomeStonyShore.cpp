#include "BiomeStonyShore.h"

BiomeStonyShore::BiomeStonyShore()
{
    m_spawnsAnimals  = false;
    m_spawnsChickens = false;
    m_topBlock       = "stone";
    m_fillerBlock    = "stone";
    m_depth          = 0.1f;
    m_scale          = 0.8f;
    m_grassColor     = 0x8AB689;
    m_foliageColor   = 0x6DA36B;
    m_skyColor       = 0x7DA2FF;
    m_fogColor       = 0xC0D8FF;
}
