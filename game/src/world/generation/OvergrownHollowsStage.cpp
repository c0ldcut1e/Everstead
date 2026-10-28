#include "OvergrownHollowsStage.h"

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/DoublePlantBlock.h"
#include "TerrainGenerator.h"

OvergrownHollowsStage::OvergrownHollowsStage(TerrainGenerator *generator)
{
    m_generator        = generator;
    m_overgrownHollows = Biome::ByName("overgrown_hollows");

    m_rubbleNoise.SetSeed((int) generator->GetSeed() ^ 0x6a2f);
    m_rubbleNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_rubbleNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_rubbleNoise.SetFractalOctaves(2);
    m_rubbleNoise.SetFrequency(1.0f / 7.0f);

    m_mossNoise.SetSeed((int) generator->GetSeed() ^ 0x1e83);
    m_mossNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_mossNoise.SetFrequency(1.0f / 10.0f);

    m_poolNoise.SetSeed((int) generator->GetSeed() ^ 0x7c15);
    m_poolNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_poolNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_poolNoise.SetFractalOctaves(2);
    m_poolNoise.SetFrequency(1.0f / 24.0f);
}

void OvergrownHollowsStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int maxVineLength = 8;

    Block *grassBlock           = Block::ByName("grass_block");
    Block *dirt                 = Block::ByName("dirt");
    Block *cobblestone          = Block::ByName("cobblestone");
    Block *mossyCobblestone     = Block::ByName("mossy_cobblestone");
    Block *mossBlock            = Block::ByName("moss_block");
    Block *mossCarpet           = Block::ByName("moss_carpet");
    Block *water                = Block::ByName("water_still");
    Block *grass                = Block::ByName("grass");
    Block *fern                 = Block::ByName("fern");
    DoublePlantBlock *tallGrass = (DoublePlantBlock *) Block::ByName("tall_grass");
    DoublePlantBlock *largeFern = (DoublePlantBlock *) Block::ByName("large_fern");
    Block *vine                 = Block::ByName("vine");
    Block *flowers[]            = {Block::ByName("blue_orchid"), Block::ByName("azure_bluet"), Block::ByName("allium")};
    Direction *sides[]          = {Direction::north, Direction::east, Direction::south, Direction::west};
    Random random(m_generator->GetChunkSeed(pos, 0x5be0cd19137e2179ULL));

    int baseX = pos.x * Chunk::s_sizeX;
    int baseZ = pos.z * Chunk::s_sizeZ;
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = 1; y < Chunk::s_sizeY - 1; y++)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_overgrownHollows || !TerrainGenerator::IsStone(Block::ById(chunk.GetBlockId(x, y, z))))
                {
                    continue;
                }

                bool exposed = chunk.GetBlockId(x, y - 1, z) == 0;
                for (int s = 0; s < 4 && !exposed; s++)
                {
                    int sx  = x + sides[s]->dx;
                    int sz  = z + sides[s]->dz;
                    exposed = sx >= 0 && sx < Chunk::s_sizeX && sz >= 0 && sz < Chunk::s_sizeZ && chunk.GetBlockId(sx, y, sz) == 0;
                }

                float rubble = m_rubbleNoise.GetNoise((float) (baseX + x), (float) y, (float) (baseZ + z));
                float moss   = m_mossNoise.GetNoise((float) (baseX + x), (float) y, (float) (baseZ + z));
                Block *block = nullptr;
                if (chunk.GetBlockId(x, y + 1, z) == 0)
                {
                    block = rubble > 0.45f ? mossyCobblestone : rubble > 0.35f ? cobblestone : moss > 0.2f ? mossBlock : grassBlock;
                }
                else if (exposed)
                {
                    if (rubble > 0.15f)
                    {
                        block = mossyCobblestone;
                    }
                    else if (rubble > 0.0f)
                    {
                        block = cobblestone;
                    }
                    else if (moss > 0.1f)
                    {
                        block = mossBlock;
                    }
                }
                else if (chunk.GetBlockId(x, y + 2, z) == 0 &&
                         (Block::ById(chunk.GetBlockId(x, y + 1, z)) == grassBlock || Block::ById(chunk.GetBlockId(x, y + 1, z)) == mossBlock))
                {
                    block = dirt;
                }

                if (block)
                {
                    chunk.SetBlock(x, y, z, block);
                    chunk.SetBlockData(x, y, z, 0);
                }
            }

            float pool = m_poolNoise.GetNoise((float) (baseX + x), (float) (baseZ + z));
            if (pool > 0.2f && x > 0 && x < Chunk::s_sizeX - 1 && z > 0 && z < Chunk::s_sizeZ - 1)
            {
                int depth = pool > 0.55f ? 3 : pool > 0.4f ? 2 : 1;
                for (int y = Chunk::s_sizeY - 2; y > depth; y--)
                {
                    Block *floor = Block::ById(chunk.GetBlockId(x, y, z));
                    if (chunk.GetBiomeAt(x, y, z) != m_overgrownHollows || chunk.GetBlockId(x, y + 1, z) != 0 || floor == water || !floor->IsSolid())
                    {
                        continue;
                    }

                    bool sealed = Block::ById(chunk.GetBlockId(x, y - depth, z))->IsSolid();
                    for (int k = 0; k < depth && sealed; k++)
                    {
                        for (int s = 0; s < 4 && sealed; s++)
                        {
                            Block *side = Block::ById(chunk.GetBlockId(x + sides[s]->dx, y - k, z + sides[s]->dz));
                            sealed      = side == water || side->IsSolid();
                        }
                    }

                    if (sealed)
                    {
                        for (int k = 0; k < depth; k++)
                        {
                            chunk.SetBlock(x, y - k, z, water);
                            chunk.SetBlockData(x, y - k, z, 0);
                        }
                    }
                }
            }
        }
    }

    if (random.NextInt(0, 3) == 0)
    {
        const int minPillarHeight = 3;
        const int maxPillarHeight = 6;
        const int minRoomHeight   = 10;

        Block *pillarBlocks[] = {Block::ByName("stone_bricks"), Block::ByName("mossy_stone_bricks"), Block::ByName("cracked_stone_bricks")};
        Block *chiseled       = Block::ByName("chiseled_stone_bricks");
        Block *lantern        = Block::ByName("orange_lantern");
        int x                 = random.NextInt(1, Chunk::s_sizeX - 2);
        int z                 = random.NextInt(1, Chunk::s_sizeZ - 2);
        int floors[Chunk::s_sizeY];
        int floorCount = 0;
        for (int y = 1; y < Chunk::s_sizeY - minRoomHeight; y++)
        {
            Block *floor = Block::ById(chunk.GetBlockId(x, y, z));
            if (chunk.GetBiomeAt(x, y + 1, z) != m_overgrownHollows || !floor->IsSolid() || chunk.GetBlockId(x, y + 1, z) != 0)
            {
                continue;
            }

            int space = 1;
            while (y + space < Chunk::s_sizeY && chunk.GetBlockId(x, y + space, z) == 0)
            {
                space++;
            }

            bool open = space > minRoomHeight;
            for (int i = 0; i < 4 && open; i++)
            {
                open = chunk.GetBlockId(x + sides[i]->dx, y + 1, z + sides[i]->dz) == 0;
            }

            if (open)
            {
                floors[floorCount++] = y;
            }
        }

        if (floorCount > 0)
        {
            int y      = floors[random.NextInt(0, floorCount - 1)];
            int height = random.NextInt(minPillarHeight, maxPillarHeight);
            for (int i = 1; i < height; i++)
            {
                chunk.SetBlock(x, y + i, z, pillarBlocks[random.NextInt(0, 2)]);
                chunk.SetBlockData(x, y + i, z, 0);
            }

            chunk.SetBlock(x, y + height, z, chiseled);
            chunk.SetBlockData(x, y + height, z, 0);
            if (random.NextInt(0, 9) != 0)
            {
                chunk.SetBlock(x, y + height + 1, z, lantern);
                chunk.SetBlockData(x, y + height + 1, z, 0);
            }
        }
    }

    // We place plants in their own pass so a pool never mistakes a neighbor plant for a wall
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = Chunk::s_sizeY - 2; y > 1; y--)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_overgrownHollows || chunk.GetBlockId(x, y, z) != 0)
                {
                    continue;
                }

                Block *below = Block::ById(chunk.GetBlockId(x, y - 1, z));
                if ((below == mossBlock || below == mossyCobblestone || below == cobblestone) && random.NextInt(0, 2) == 0)
                {
                    chunk.SetBlock(x, y, z, below == mossBlock && random.NextInt(0, 1) == 0 ? grass : mossCarpet);
                    chunk.SetBlockData(x, y, z, 0);
                    continue;
                }

                if (below == grassBlock)
                {
                    int roll               = random.NextInt(0, 99);
                    DoublePlantBlock *tall = roll < 35 ? tallGrass : roll < 42 ? largeFern : nullptr;
                    if (tall && chunk.GetBlockId(x, y + 1, z) == 0)
                    {
                        chunk.SetBlock(x, y, z, tall);
                        chunk.SetBlockData(x, y, z, 0);
                        chunk.SetBlock(x, y + 1, z, tall);
                        chunk.SetBlockData(x, y + 1, z, tall->GetUpperData());
                    }
                    else if (roll >= 42 && roll < 82)
                    {
                        chunk.SetBlock(x, y, z, roll < 72 ? grass : fern);
                        chunk.SetBlockData(x, y, z, 0);
                    }
                    else if (roll == 99 && random.NextInt(0, 2) == 0)
                    {
                        chunk.SetBlock(x, y, z, flowers[random.NextInt(0, 2)]);
                        chunk.SetBlockData(x, y, z, 0);
                    }

                    continue;
                }

                if (random.NextInt(0, 5) != 0)
                {
                    continue;
                }

                for (int s = 0; s < 4; s++)
                {
                    int sx = x + sides[s]->dx;
                    int sz = z + sides[s]->dz;
                    if (sx < 0 || sx >= Chunk::s_sizeX || sz < 0 || sz >= Chunk::s_sizeZ || !Block::ById(chunk.GetBlockId(sx, y, sz))->IsSolid() ||
                        !Block::ById(chunk.GetBlockId(sx, y + 1, sz))->IsSolid() || !Block::ById(chunk.GetBlockId(x, y + 1, z))->IsSolid())
                    {
                        continue;
                    }

                    uint16_t data = vine->GetPlacementData(Direction::Opposite(sides[s]));
                    int length    = random.NextInt(2, maxVineLength);
                    for (int i = 0;
                         i < length && y - i > 0 && chunk.GetBlockId(x, y - i, z) == 0 && Block::ById(chunk.GetBlockId(sx, y - i, sz))->IsSolid();
                         i++)
                    {
                        chunk.SetBlock(x, y - i, z, vine);
                        chunk.SetBlockData(x, y - i, z, data);
                    }

                    break;
                }
            }
        }
    }
}
