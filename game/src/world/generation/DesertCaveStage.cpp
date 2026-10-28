#include "DesertCaveStage.h"

#include <algorithm>
#include <iterator>
#include <string>

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/PricklyVineBlock.h"
#include "TerrainGenerator.h"

DesertCaveStage::DesertCaveStage(TerrainGenerator *generator)
{
    m_generator   = generator;
    m_desertCaves = Biome::ByName("desert_caves");
}

void DesertCaveStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int maxVineLength = 4;

    Block *sand                   = Block::ByName("sand");
    Block *sandstone              = Block::ByName("sandstone");
    Block *cactus                 = Block::ByName("cactus");
    Block *sunbloom               = Block::ByName("sunbloom");
    PricklyVineBlock *pricklyVine = (PricklyVineBlock *) Block::ByName("prickly_vine");
    Direction *sunbloomSides[]    = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};
    const char *oreNames[]        = {"coal", "iron", "gold", "diamond", "emerald", "lapis", "redstone"};
    Block *stoneOres[std::size(oreNames)];
    Block *sandstoneOres[std::size(oreNames)];
    for (size_t i = 0; i < std::size(oreNames); i++)
    {
        stoneOres[i]     = Block::ByName(std::string(oreNames[i]) + "_ore");
        sandstoneOres[i] = Block::ByName(std::string("sandstone_") + oreNames[i] + "_ore");
    }

    Random random(m_generator->GetChunkSeed(pos, 0x3c6ef372fe94f82bULL));

    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = 1; y < Chunk::s_sizeY - 1; y++)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_desertCaves)
                {
                    continue;
                }

                Block *block = Block::ById(chunk.GetBlockId(x, y, z));
                Block **ore  = std::find(stoneOres, stoneOres + std::size(oreNames), block);
                if (ore != stoneOres + std::size(oreNames))
                {
                    chunk.SetBlock(x, y, z, sandstoneOres[ore - stoneOres]);
                    continue;
                }

                if (!TerrainGenerator::IsStone(block))
                {
                    continue;
                }

                if (chunk.GetBlockId(x, y + 1, z) == 0)
                {
                    chunk.SetBlock(x, y, z, sand);
                }
                else if (random.NextInt(0, 7) != 0)
                {
                    chunk.SetBlock(x, y, z, sandstone);
                }
                else
                {
                    continue;
                }

                chunk.SetBlockData(x, y, z, 0);
            }

            for (int y = Chunk::s_sizeY - 2; y > 1; y--)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_desertCaves || chunk.GetBlockId(x, y, z) != 0)
                {
                    continue;
                }

                Block *above = Block::ById(chunk.GetBlockId(x, y + 1, z));
                Block *below = Block::ById(chunk.GetBlockId(x, y - 1, z));
                if ((above == sandstone || TerrainGenerator::IsStone(above)) && random.NextInt(0, 11) == 0)
                {
                    int space = 0;
                    while (space < maxVineLength + 1 && y - space > 0 && chunk.GetBlockId(x, y - space, z) == 0)
                    {
                        space++;
                    }

                    int length = std::min(random.NextInt(1, maxVineLength), space - 1);
                    for (int i = 0; i < length; i++)
                    {
                        chunk.SetBlock(x, y - i, z, pricklyVine);
                        chunk.SetBlockData(x, y - i, z, pricklyVine->MakeState(i == length - 1));
                    }
                }
                else if (below == sand && random.NextInt(0, 79) == 0 && x > 0 && x < Chunk::s_sizeX - 1 && z > 0 && z < Chunk::s_sizeZ - 1)
                {
                    int height = random.NextInt(1, 3);
                    bool clear = true;
                    for (int i = 0; i < height + 1 && clear; i++)
                    {
                        clear = chunk.GetBlockId(x, y + i, z) == 0 && chunk.GetBlockId(x - 1, y + i, z) == 0 &&
                                chunk.GetBlockId(x + 1, y + i, z) == 0 && chunk.GetBlockId(x, y + i, z - 1) == 0 &&
                                chunk.GetBlockId(x, y + i, z + 1) == 0;
                    }

                    if (clear)
                    {
                        for (int i = 0; i < height; i++)
                        {
                            chunk.SetBlock(x, y + i, z, cactus);
                            chunk.SetBlockData(x, y + i, z, 0);
                        }
                    }
                }
                else if (random.NextInt(0, 179) == 0)
                {
                    Direction *supports[6];
                    int supportCount = 0;
                    for (int i = 0; i < 6; i++)
                    {
                        Direction *side = sunbloomSides[i];
                        int sx          = x + side->dx;
                        int sz          = z + side->dz;
                        if (sx < 0 || sx >= Chunk::s_sizeX || sz < 0 || sz >= Chunk::s_sizeZ)
                        {
                            continue;
                        }

                        Block *support = Block::ById(chunk.GetBlockId(sx, y + side->dy, sz));
                        if (support == sand || support == sandstone)
                        {
                            supports[supportCount++] = side;
                        }
                    }

                    if (supportCount > 0)
                    {
                        Direction *side = supports[random.NextInt(0, supportCount - 1)];
                        chunk.SetBlock(x, y, z, sunbloom);
                        chunk.SetBlockData(x, y, z, sunbloom->GetPlacementData(Direction::Opposite(side)));
                    }
                }
            }
        }
    }
}
