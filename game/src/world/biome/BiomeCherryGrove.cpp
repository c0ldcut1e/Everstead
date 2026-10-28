#include "BiomeCherryGrove.h"

BiomeCherryGrove::BiomeCherryGrove()
{
    m_topBlock      = "grass_block";
    m_fillerBlock   = "dirt";
    m_depth         = 0.45f;
    m_scale         = 0.35f;
    m_mountainDepth = 4.0f;
    m_grassColor    = 0xB6DB61;
    m_foliageColor  = 0xB6DB61;
    m_skyColor      = 0x80AAFF;
    m_fogColor      = 0xC4D8FC;

    m_decorated           = true;
    m_treeDensity         = 2.0f;
    m_treeType            = eTreeType_CHERRY;
    m_grassPatches        = 10;
    m_noiseFlowers        = {"pink_petals"};
    m_noiseFlowerAttempts = 48;
}
