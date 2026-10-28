#include "JungleStage.h"

#include "../../utils/Direction.h"
#include "../biome/Biome.h"
#include "../block/BambooBlock.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"

JungleStage::JungleStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void JungleStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int vineHeight = 48;
    const int minBamboo  = 5;
    const int maxBamboo  = 14;

    Block *air          = Block::ByName("air");
    Block *vine         = Block::ByName("vine");
    Block *sapling      = Block::ByName("bamboo_sapling");
    BambooBlock *bamboo = (BambooBlock *) Block::ByName("bamboo");
    Random random(m_generator->GetChunkSeed(pos, 0x6a09e667bb67ae85ULL));

    Biome *biome = chunk.GetSurfaceBiome(Chunk::s_sizeX / 2, Chunk::s_sizeZ / 2);
    int chunkX   = pos.x * Chunk::s_sizeX;
    int chunkZ   = pos.z * Chunk::s_sizeZ;
    for (int i = 0; i < biome->GetBambooAttempts(); i++)
    {
        int x      = random.NextInt(0, Chunk::s_sizeX - 1);
        int z      = random.NextInt(0, Chunk::s_sizeZ - 1);
        int height = random.NextInt(minBamboo, maxBamboo);
        int age    = random.NextInt(0, 1);
        bool young = random.NextInt(0, 3) == 0;
        int y      = m_generator->GetTerrainHeight(chunkX + x, chunkZ + z);
        if (!BambooBlock::CanPlantOn(Block::ById(chunk.GetBlockId(x, y, z))) || Block::ById(chunk.GetBlockId(x, y + 1, z)) != air)
        {
            continue;
        }

        if (young)
        {
            chunk.SetBlock(x, y + 1, z, sapling);
            continue;
        }

        for (int h = 1; h <= height && y + h < Chunk::s_sizeY && Block::ById(chunk.GetBlockId(x, y + h, z)) == air; h++)
        {
            BambooBlock::eLeaves leaves = BambooBlock::eLeaves_None;
            if (h >= height - 1)
            {
                leaves = BambooBlock::eLeaves_Large;
            }
            else if (h == height - 2)
            {
                leaves = BambooBlock::eLeaves_Small;
            }

            chunk.SetBlock(x, y + h, z, bamboo);
            chunk.SetBlockData(x, y + h, z, bamboo->GetStateData(age, leaves, 0));
        }
    }

    Direction *sides[] = {Direction::north, Direction::east, Direction::south, Direction::west};
    for (int i = 0; i < biome->GetVineAttempts(); i++)
    {
        int x       = random.NextInt(0, Chunk::s_sizeX - 1);
        int z       = random.NextInt(0, Chunk::s_sizeZ - 1);
        int surface = m_generator->GetTerrainHeight(chunkX + x, chunkZ + z);
        for (int y = surface + 1; y < surface + vineHeight && y < Chunk::s_sizeY; y++)
        {
            if (Block::ById(chunk.GetBlockId(x, y, z)) != air)
            {
                continue;
            }

            for (int s = 0; s < 4; s++)
            {
                int sx = x + sides[s]->dx;
                int sz = z + sides[s]->dz;
                if (sx < 0 || sx >= Chunk::s_sizeX || sz < 0 || sz >= Chunk::s_sizeZ || !Block::ById(chunk.GetBlockId(sx, y, sz))->IsSolid())
                {
                    continue;
                }

                chunk.SetBlock(x, y, z, vine);
                chunk.SetBlockData(x, y, z, vine->GetPlacementData(Direction::Opposite(sides[s])));
                break;
            }
        }
    }
}
