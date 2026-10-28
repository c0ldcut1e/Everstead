#include "CaveDimensionBiomeStage.h"

#include "../biome/Biome.h"
#include "TerrainGenerator.h"

CaveDimensionBiomeStage::CaveDimensionBiomeStage(uint32_t seed)
{
    m_cave             = Biome::ByName("cave");
    m_dripstoneCaves   = Biome::ByName("dripstone_caves");
    m_iceCaves         = Biome::ByName("ice_caves");
    m_desertCaves      = Biome::ByName("desert_caves");
    m_overgrownHollows = Biome::ByName("overgrown_hollows");
    m_magmaDepths      = Biome::ByName("magma_depths");

    m_temperatureNoise.SetSeed((int) seed ^ 0x6c3e);
    m_temperatureNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_temperatureNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_temperatureNoise.SetFractalOctaves(2);
    m_temperatureNoise.SetFrequency(1.0f);

    m_vegetationNoise.SetSeed((int) seed ^ 0x2b85);
    m_vegetationNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_vegetationNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_vegetationNoise.SetFractalOctaves(2);
    m_vegetationNoise.SetFrequency(1.0f);
}

void CaveDimensionBiomeStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const float scaleXZ       = 1.0f / 192.0f;
    const float scaleY        = 1.0f / 96.0f;
    const float extremeCutoff = 0.3f;
    const float mildCutoff    = 0.25f;

    int baseX = pos.x * Chunk::s_sizeX;
    int baseZ = pos.z * Chunk::s_sizeZ;
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = 0; y < TerrainGenerator::s_caveCeiling; y += ChunkSection::s_biomeHeight)
            {
                float temperature = m_temperatureNoise.GetNoise((baseX + x) * scaleXZ, y * scaleY, (baseZ + z) * scaleXZ);
                float vegetation  = m_vegetationNoise.GetNoise((baseX + x) * scaleXZ, y * scaleY, (baseZ + z) * scaleXZ);
                Biome *biome      = m_cave;
                if (temperature < -extremeCutoff)
                {
                    biome = m_iceCaves;
                }
                else if (temperature > extremeCutoff)
                {
                    biome = vegetation > 0.0f ? m_desertCaves : m_magmaDepths;
                }
                else if (vegetation > mildCutoff)
                {
                    biome = m_overgrownHollows;
                }
                else if (vegetation < -mildCutoff)
                {
                    biome = m_dripstoneCaves;
                }

                chunk.SetBiomeAt(x, y, z, biome);
            }
        }
    }
}
