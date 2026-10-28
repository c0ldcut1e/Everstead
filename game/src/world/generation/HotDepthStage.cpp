#include "HotDepthStage.h"

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"
#include "VeinFeature.h"

HotDepthStage::HotDepthStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void HotDepthStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int hotDepth         = 20;
    const int pocketsPerDraw   = 2;
    const int pocketSize       = 12;
    const int brimstoneSize    = 10;
    const float lavaChance     = 0.6f;
    const float crustChance    = 0.3f;
    const float brimstoneRatio = 0.5f;

    Block *magma     = Block::ByName("magma");
    Block *lava      = Block::ByName("lava_still");
    Block *brimstone = Block::ByName("brimstone");
    Block *stone     = Block::ByName("stone");
    for (int ownerX = pos.x - 1; ownerX <= pos.x + 1; ownerX++)
    {
        for (int ownerZ = pos.z - 1; ownerZ <= pos.z + 1; ownerZ++)
        {
            ChunkPos owner(ownerX, ownerZ);
            Random random(m_generator->GetChunkSeed(owner, 0x1f83d9abfb41bd6bULL));
            for (int pocket = 0; pocket < pocketsPerDraw; pocket++)
            {
                int x = ownerX * Chunk::s_sizeX + random.NextInt(0, Chunk::s_sizeX - 1);
                int z = ownerZ * Chunk::s_sizeZ + random.NextInt(0, Chunk::s_sizeZ - 1);
                int y = random.NextInt(1, hotDepth / 2);
                VeinFeature::Place(chunk, pos, random, x, y, z, magma, stone, pocketSize);
            }

            Random brimstoneRandom(m_generator->GetChunkSeed(owner, 0x3c6ef372fe94f82bULL));
            int x = ownerX * Chunk::s_sizeX + brimstoneRandom.NextInt(0, Chunk::s_sizeX - 1);
            int z = ownerZ * Chunk::s_sizeZ + brimstoneRandom.NextInt(0, Chunk::s_sizeZ - 1);
            int y = brimstoneRandom.NextInt(1, hotDepth / 2);
            VeinFeature::Place(chunk, pos, brimstoneRandom, x, y, z, brimstone, stone, brimstoneSize);
        }
    }

    Random random(m_generator->GetChunkSeed(pos, 0x5be0cd19137e2179ULL));
    Random brimstoneRandom(m_generator->GetChunkSeed(pos, 0x510e527fade682d1ULL));
    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int y = 1; y < hotDepth; y++)
            {
                if (!TerrainGenerator::IsStone(Block::ById(chunk.GetBlockId(x, y, z))))
                {
                    continue;
                }

                bool exposed     = false;
                bool touchesLava = false;
                for (int id = 1; id <= 6; id++)
                {
                    Direction *side = Direction::ById(id);
                    int nx          = x + side->dx;
                    int nz          = z + side->dz;
                    if (nx < 0 || nx >= Chunk::s_sizeX || nz < 0 || nz >= Chunk::s_sizeZ)
                    {
                        continue;
                    }

                    Block *neighbor = Block::ById(chunk.GetBlockId(nx, y + side->dy, nz));
                    exposed         = exposed || chunk.GetBlockId(nx, y + side->dy, nz) == 0;
                    touchesLava     = touchesLava || neighbor == lava;
                }

                float heat   = (float) (hotDepth - y) / (float) hotDepth;
                float chance = touchesLava ? lavaChance : (exposed ? heat * heat * crustChance : 0.0f);
                if (random.NextFloat() < chance)
                {
                    chunk.SetBlock(x, y, z, magma);
                }
                else if (brimstoneRandom.NextFloat() < chance * brimstoneRatio)
                {
                    chunk.SetBlock(x, y, z, brimstone);
                }
            }
        }
    }
}
