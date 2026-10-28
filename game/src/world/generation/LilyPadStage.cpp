#include "LilyPadStage.h"

#include "../biome/Biome.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"

LilyPadStage::LilyPadStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void LilyPadStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int triesPerPatch = 10;

    Block *air        = Block::ByName("air");
    Block *waterStill = Block::ByName("water_still");
    Block *lilyPad    = Block::ByName("lily_pad");
    Random random(m_generator->GetChunkSeed(pos, 0x9b05688c2b3e6c1fULL));

    int y       = TerrainGenerator::s_seaLevel;
    int patches = chunk.GetSurfaceBiome(Chunk::s_sizeX / 2, Chunk::s_sizeZ / 2)->GetLilyPads();
    for (int patch = 0; patch < patches; patch++)
    {
        int centerX = random.NextInt(0, Chunk::s_sizeX - 1);
        int centerZ = random.NextInt(0, Chunk::s_sizeZ - 1);
        for (int i = 0; i < triesPerPatch; i++)
        {
            int x = centerX + random.NextInt(0, 7) - random.NextInt(0, 7);
            int z = centerZ + random.NextInt(0, 7) - random.NextInt(0, 7);
            if (x < 0 || x >= Chunk::s_sizeX || z < 0 || z >= Chunk::s_sizeZ || Block::ById(chunk.GetBlockId(x, y, z)) != air ||
                Block::ById(chunk.GetBlockId(x, y - 1, z)) != waterStill)
            {
                continue;
            }

            chunk.SetBlock(x, y, z, lilyPad);
        }
    }
}
