#include "CaveBiomeStage.h"

#include <algorithm>

#include "../block/Block.h"
#include "TerrainGenerator.h"

CaveBiomeStage::CaveBiomeStage(TerrainGenerator *generator)
{
    m_generator        = generator;
    m_cave             = Biome::ByName("cave");
    m_dripstoneCaves   = Biome::ByName("dripstone_caves");
    m_iceCaves         = Biome::ByName("ice_caves");
    m_desertCaves      = Biome::ByName("desert_caves");
    m_overgrownHollows = Biome::ByName("overgrown_hollows");
    m_glacier          = Biome::ByName("glacier");
    m_magmaDepths      = Biome::ByName("magma_depths");

    m_dripstoneNoise.SetSeed((int) generator->GetSeed() ^ 0x3d91);
    m_dripstoneNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_dripstoneNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_dripstoneNoise.SetFractalOctaves(2);
    m_dripstoneNoise.SetFrequency(1.0f);

    m_iceCaveNoise.SetSeed((int) generator->GetSeed() ^ 0x51c7);
    m_iceCaveNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_iceCaveNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_iceCaveNoise.SetFractalOctaves(2);
    m_iceCaveNoise.SetFrequency(1.0f);

    m_magmaNoise.SetSeed((int) generator->GetSeed() ^ 0x7a2d);
    m_magmaNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_magmaNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_magmaNoise.SetFractalOctaves(2);
    m_magmaNoise.SetFrequency(1.0f);
}

// Biome cells fully caveDepth blocks under the solid surface become cave, so the sea itself stays ocean
void CaveBiomeStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int caveDepth          = 8;
    const float dripstoneScaleXZ = 1.0f / 128.0f;
    const float dripstoneScaleY  = 1.0f / 64.0f;
    const float dripstoneCutoff  = 0.25f;
    const float iceSpreadFalloff = 20.0f;
    const int magmaTop           = 32;
    const float magmaCutoff      = 0.3f;

    int baseX = pos.x * Chunk::s_sizeX;
    int baseZ = pos.z * Chunk::s_sizeZ;
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            int surface = Chunk::s_sizeY - 1;
            while (surface >= 0)
            {
                uint32_t id = chunk.GetBlockId(x, surface, z);
                if (id != 0 && !Block::ById(id)->IsLiquid())
                {
                    break;
                }

                surface--;
            }

            int caveTop = surface - caveDepth;

            float snowMargin    = m_generator->GetBiomeSource().GetSnowMargin(baseX + x, baseZ + z);
            float iceCutoff     = dripstoneCutoff + std::max(snowMargin, 0.0f) * iceSpreadFalloff;
            Biome *surfaceBiome = chunk.GetSurfaceBiome(x, z);
            if (!surfaceBiome->HasIceCaves())
            {
                iceCutoff = 2.0f;
            }

            bool glacier   = surfaceBiome == m_glacier;
            bool hot       = surfaceBiome->HasHotCaves();
            bool overgrown = surfaceBiome->HasOvergrownCaves();
            for (int y = 0; y + ChunkSection::s_biomeHeight - 1 <= caveTop; y += ChunkSection::s_biomeHeight)
            {
                float dripstone = m_dripstoneNoise.GetNoise((baseX + x) * dripstoneScaleXZ, y * dripstoneScaleY, (baseZ + z) * dripstoneScaleXZ);
                float magma     = m_magmaNoise.GetNoise((baseX + x) * dripstoneScaleXZ, (baseZ + z) * dripstoneScaleXZ);
                float ice       = m_iceCaveNoise.GetNoise((baseX + x) * dripstoneScaleXZ, y * dripstoneScaleY, (baseZ + z) * dripstoneScaleXZ);
                Biome *biome    = m_cave;
                if (glacier)
                {
                    biome = m_iceCaves;
                }
                else if (overgrown)
                {
                    biome = m_overgrownHollows;
                }
                else if (y < magmaTop && magma > magmaCutoff)
                {
                    biome = m_magmaDepths;
                }
                else if (dripstone > dripstoneCutoff)
                {
                    biome = m_dripstoneCaves;
                }
                else if (ice > iceCutoff)
                {
                    biome = m_iceCaves;
                }
                else if (hot)
                {
                    biome = m_desertCaves;
                }

                chunk.SetBiomeAt(x, y, z, biome);
            }
        }
    }
}
