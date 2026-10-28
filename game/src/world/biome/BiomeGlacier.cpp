#include "BiomeGlacier.h"

BiomeGlacier::BiomeGlacier()
{
    m_topBlock     = "snow_block";
    m_fillerBlock  = "packed_ice";
    m_depth        = 0.3f;
    m_scale        = 0.08f;
    m_grassColor   = 0x80B497;
    m_foliageColor = 0x60A17B;
    m_snowy        = true;
    m_skyColor     = 0x8CB0F8;
    m_fogColor     = 0xD4E2F8;

    m_iceCaves = true;
}
