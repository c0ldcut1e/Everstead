#include "BiomeSnowyTundra.h"

BiomeSnowyTundra::BiomeSnowyTundra()
{
    m_topBlock     = "grass_block";
    m_fillerBlock  = "dirt";
    m_depth        = 0.125f;
    m_scale        = 0.05f;
    m_grassColor   = 0x80B497;
    m_foliageColor = 0x60A17B;
    m_snowy        = true;
    m_skyColor     = 0x8CB0F8;
    m_fogColor     = 0xD4E2F8;

    m_decorated   = true;
    m_treeDensity = 1.0f / 3.0f;
    m_treeType    = eTreeType_SPRUCE;
}
