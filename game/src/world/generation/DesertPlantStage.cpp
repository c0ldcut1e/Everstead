#include "DesertPlantStage.h"

#include "../biome/Biome.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"

static bool IsCactusSpotFree(const Chunk &chunk, Block *air, int x, int y, int z)
{
    if (Block::ById(chunk.GetBlockId(x, y, z)) != air)
    {
        return false;
    }

    const int offsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < 4; i++)
    {
        if (Block::ById(chunk.GetBlockId(x + offsets[i][0], y, z + offsets[i][1])) != air)
        {
            return false;
        }
    }

    return true;
}

DesertPlantStage::DesertPlantStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void DesertPlantStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int cactusPatchRarity = 6;
    const int cactusPatchTries  = 10;
    const int cactusPatchSpread = 4;
    const int deadBushAttempts  = 1;
    const int maxCactusHeight   = 3;

    Block *air      = Block::ByName("air");
    Block *sand     = Block::ByName("sand");
    Block *redSand  = Block::ByName("red_sand");
    Block *cactus   = Block::ByName("cactus");
    Block *deadBush = Block::ByName("dead_bush");
    Random random(m_generator->GetChunkSeed(pos, 0x6a09e667f3bcc909ULL));

    if (random.NextInt(0, cactusPatchRarity - 1) == 0)
    {
        // We stay one block in from the edge so the side checks never leave the chunk
        int centerX = random.NextInt(1 + cactusPatchSpread, Chunk::s_sizeX - 2 - cactusPatchSpread);
        int centerZ = random.NextInt(1 + cactusPatchSpread, Chunk::s_sizeZ - 2 - cactusPatchSpread);
        for (int attempt = 0; attempt < cactusPatchTries; attempt++)
        {
            int x = centerX + random.NextInt(-cactusPatchSpread, cactusPatchSpread);
            int z = centerZ + random.NextInt(-cactusPatchSpread, cactusPatchSpread);
            if (!chunk.GetSurfaceBiome(x, z)->HasDesertPlants())
            {
                continue;
            }

            int ground   = TerrainGenerator::FindTopSolid(chunk, x, z);
            Block *floor = ground >= 0 ? Block::ById(chunk.GetBlockId(x, ground, z)) : nullptr;
            if (floor != sand && floor != redSand)
            {
                continue;
            }

            int height = 1 + random.NextInt(0, random.NextInt(0, maxCactusHeight - 1));
            for (int i = 1; i <= height; i++)
            {
                if (!IsCactusSpotFree(chunk, air, x, ground + i, z))
                {
                    break;
                }

                chunk.SetBlock(x, ground + i, z, cactus);
            }
        }
    }

    for (int attempt = 0; attempt < deadBushAttempts; attempt++)
    {
        int x = random.NextInt(0, Chunk::s_sizeX - 1);
        int z = random.NextInt(0, Chunk::s_sizeZ - 1);
        if (!chunk.GetSurfaceBiome(x, z)->HasDesertPlants())
        {
            continue;
        }

        int ground   = TerrainGenerator::FindTopSolid(chunk, x, z);
        Block *floor = ground >= 0 ? Block::ById(chunk.GetBlockId(x, ground, z)) : nullptr;
        if ((floor == sand || floor == redSand) && Block::ById(chunk.GetBlockId(x, ground + 1, z)) == air)
        {
            chunk.SetBlock(x, ground + 1, z, deadBush);
        }
    }
}
