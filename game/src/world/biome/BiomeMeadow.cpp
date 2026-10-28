#include "BiomeMeadow.h"

BiomeMeadow::BiomeMeadow()
{
    m_topBlock      = "grass_block";
    m_fillerBlock   = "dirt";
    m_depth         = 0.45f;
    m_scale         = 0.15f;
    m_mountainDepth = 4.0f;
    m_grassColor    = 0x83BB6D;
    m_foliageColor  = 0x63A948;
    m_skyColor      = 0x80AAFF;
    m_fogColor      = 0xC4D8FC;

    m_decorated           = true;
    m_treeDensity         = 1.0f / 60.0f;
    m_grassPatches        = 15;
    m_noiseFlowers        = {"allium", "poppy", "azure_bluet", "dandelion", "oxeye_daisy"};
    m_noiseFlowerAttempts = 48;
}
