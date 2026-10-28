#include "BiomeBeach.h"

BiomeBeach::BiomeBeach()
{
    m_spawnsAnimals  = false;
    m_spawnsChickens = false;
    m_topBlock       = "sand";
    m_fillerBlock    = "sand";
    m_depth          = 0.0f;
    m_scale          = 0.025f;
    m_grassColor     = 0x91BD59;
    m_foliageColor   = 0x77AB2F;
    m_skyColor       = 0x82AAFF;
    m_fogColor       = 0xC6D8FC;
}
