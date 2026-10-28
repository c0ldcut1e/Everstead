#include "DripstoneStage.h"

#include <algorithm>

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/PointedDripstoneBlock.h"
#include "TerrainGenerator.h"

DripstoneStage::DripstoneStage(TerrainGenerator *generator)
{
    m_generator      = generator;
    m_dripstoneCaves = Biome::ByName("dripstone_caves");
}

void DripstoneStage::PlaceColumn(Chunk &chunk, PointedDripstoneBlock *block, int x, int y, int z, Direction *tipDirection, int length)
{
    for (int i = 0; i < length; i++)
    {
        int thickness = PointedDripstoneBlock::eThickness_MIDDLE;
        if (i == length - 1)
        {
            thickness = PointedDripstoneBlock::eThickness_TIP;
        }
        else if (i == length - 2)
        {
            thickness = PointedDripstoneBlock::eThickness_FRUSTUM;
        }
        else if (i == 0)
        {
            thickness = PointedDripstoneBlock::eThickness_BASE;
        }

        int blockY = y + tipDirection->dy * i;
        chunk.SetBlock(x, blockY, z, block);
        chunk.SetBlockData(x, blockY, z, block->MakeState(tipDirection, thickness));
    }
}

void DripstoneStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int blockChance      = 3;
    const int stalactiteChance = 6;
    const int stalagmiteChance = 9;
    const int maxLength        = 5;

    Block *dripstoneBlock          = Block::ByName("dripstone_block");
    PointedDripstoneBlock *pointed = (PointedDripstoneBlock *) Block::ByName("pointed_dripstone");
    Random random(m_generator->GetChunkSeed(pos, 0x9b05688c2b3e6c1fULL));
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = 1; y < Chunk::s_sizeY - 1; y++)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_dripstoneCaves || !TerrainGenerator::IsStone(Block::ById(chunk.GetBlockId(x, y, z))))
                {
                    continue;
                }

                bool exposed = chunk.GetBlockId(x, y + 1, z) == 0 || chunk.GetBlockId(x, y - 1, z) == 0;
                if (exposed || random.NextInt(0, blockChance - 1) == 0)
                {
                    chunk.SetBlock(x, y, z, dripstoneBlock);
                }
            }

            for (int y = Chunk::s_sizeY - 2; y > 1; y--)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_dripstoneCaves || chunk.GetBlockId(x, y, z) != 0)
                {
                    continue;
                }

                Block *above = Block::ById(chunk.GetBlockId(x, y + 1, z));
                Block *below = Block::ById(chunk.GetBlockId(x, y - 1, z));
                if (above == dripstoneBlock && random.NextInt(0, stalactiteChance - 1) == 0)
                {
                    int space = 0;
                    while (space < maxLength + 1 && y - space > 0 && chunk.GetBlockId(x, y - space, z) == 0)
                    {
                        space++;
                    }

                    int length = std::min(random.NextInt(1, maxLength), space - 1);
                    if (length > 0)
                    {
                        PlaceColumn(chunk, pointed, x, y, z, Direction::down, length);
                    }
                }
                else if (below == dripstoneBlock && random.NextInt(0, stalagmiteChance - 1) == 0)
                {
                    int space = 0;
                    while (space < maxLength + 1 && y + space < Chunk::s_sizeY - 1 && chunk.GetBlockId(x, y + space, z) == 0)
                    {
                        space++;
                    }

                    int length = std::min(random.NextInt(1, maxLength - 2), space - 1);
                    if (length > 0)
                    {
                        PlaceColumn(chunk, pointed, x, y, z, Direction::up, length);
                    }
                }
            }
        }
    }
}
