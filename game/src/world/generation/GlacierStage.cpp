#include "GlacierStage.h"

#include <algorithm>
#include <cmath>

#include "../block/Block.h"
#include "NoiseCaveCarver.h"
#include "TerrainGenerator.h"

GlacierStage::GlacierStage(TerrainGenerator *generator)
{
    m_glacier   = Biome::ByName("glacier");
    m_air       = Block::ByName("air");
    m_packedIce = Block::ByName("packed_ice");

    m_crevasseNoise.SetSeed((int) generator->GetSeed() ^ 0x61ac);
    m_crevasseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_crevasseNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_crevasseNoise.SetFractalOctaves(2);
    m_crevasseNoise.SetFrequency(1.0f);

    m_depthNoise.SetSeed((int) generator->GetSeed() ^ 0x61ad);
    m_depthNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_depthNoise.SetFrequency(1.0f);
}

// Crevasses follow the zero line of a noise field and taper with depth, so they can open into the ice caves below
void GlacierStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const float crevasseScale = 1.0f / 56.0f;
    const float depthScale    = 1.0f / 40.0f;
    const float crevasseWidth = 0.06f;
    const float wallWidth     = 1.8f;
    const int minDepth        = 16;
    const int depthRange      = 36;
    const int minBottom       = NoiseCaveCarver::s_lavaLevel + 8;

    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            if (chunk.GetSurfaceBiome(x, z) != m_glacier)
            {
                continue;
            }

            float levelX = (float) (pos.x * Chunk::s_sizeX + x);
            float levelZ = (float) (pos.z * Chunk::s_sizeZ + z);
            float ridge  = std::abs(m_crevasseNoise.GetNoise(levelX * crevasseScale, levelZ * crevasseScale));
            if (ridge >= crevasseWidth * wallWidth)
            {
                continue;
            }

            int surface = Chunk::s_sizeY - 1;
            while (surface > 0 && chunk.GetBlockId(x, surface, z) == 0)
            {
                surface--;
            }

            if (Block::ById(chunk.GetBlockId(x, surface, z))->IsLiquid())
            {
                continue;
            }

            int depth  = minDepth + (int) ((m_depthNoise.GetNoise(levelX * depthScale, levelZ * depthScale) + 1.0f) * 0.5f * depthRange);
            int bottom = std::max(surface - depth, minBottom);
            for (int y = surface; y >= bottom; y--)
            {
                uint32_t id = chunk.GetBlockId(x, y, z);
                if (id == 0)
                {
                    continue;
                }

                Block *block = Block::ById(id);
                if (block->IsLiquid())
                {
                    break;
                }

                float width = crevasseWidth * (1.0f - 0.75f * (float) (surface - y) / (float) depth);
                if (ridge < width)
                {
                    chunk.SetBlock(x, y, z, m_air);
                }
                else if (ridge < width * wallWidth && TerrainGenerator::IsStone(block))
                {
                    chunk.SetBlock(x, y, z, m_packedIce);
                }
            }
        }
    }
}
