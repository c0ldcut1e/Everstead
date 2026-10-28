#include "BadlandsStage.h"

#include <string>

#include "../../utils/Random.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"

BadlandsStage::BadlandsStage(TerrainGenerator *generator)
{
    m_badlands       = Biome::ByName("badlands");
    m_woodedBadlands = Biome::ByName("wooded_badlands");

    Block *terracotta = Block::ByName("terracotta");
    for (int i = 0; i < s_bandCount; i++)
    {
        m_bands[i] = terracotta;
    }

    const char *colors[]  = {"orange", "yellow", "brown", "red", "white", "light_gray"};
    const int runs[]      = {12, 4, 4, 4, 3, 3};
    const int maxLength[] = {1, 3, 3, 3, 1, 1};
    Random random((uint64_t) generator->GetSeed() ^ 0x6a09e667f3bcc908ULL);
    for (int i = 0; i < 6; i++)
    {
        Block *band = Block::ByName(std::string(colors[i]) + "_terracotta");
        for (int run = 0; run < runs[i]; run++)
        {
            int start  = random.NextInt(0, s_bandCount - 1);
            int length = random.NextInt(1, maxLength[i]);
            for (int y = start; y < start + length; y++)
            {
                m_bands[y % s_bandCount] = band;
            }
        }
    }

    m_bandNoise.SetSeed((int) generator->GetSeed() ^ 0x2b4f);
    m_bandNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_bandNoise.SetFrequency(1.0f / 128.0f);
}

void BadlandsStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int bandDepth = 24;
    const float shift   = 3.0f;

    Block *terracotta = Block::ByName("terracotta");
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            Biome *biome = chunk.GetSurfaceBiome(x, z);
            if (biome != m_badlands && biome != m_woodedBadlands)
            {
                continue;
            }

            int top    = TerrainGenerator::FindTopSolid(chunk, x, z);
            int offset = (int) (m_bandNoise.GetNoise((float) (pos.x * Chunk::s_sizeX + x), (float) (pos.z * Chunk::s_sizeZ + z)) * shift);
            for (int y = top; y > top - bandDepth && y > 0; y--)
            {
                Block *block = Block::ById(chunk.GetBlockId(x, y, z));
                if (block == terracotta || TerrainGenerator::IsStone(block))
                {
                    chunk.SetBlock(x, y, z, m_bands[(y + offset + s_bandCount * 4) % s_bandCount]);
                    chunk.SetBlockData(x, y, z, 0);
                }
            }
        }
    }
}
