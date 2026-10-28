#include "SugarCaneStage.h"

#include "../biome/Biome.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"

SugarCaneStage::SugarCaneStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void SugarCaneStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int extraAttempts = 10;
    const int triesPerPatch = 20;
    const int genDepth      = 128;

    Block *air        = Block::ByName("air");
    Block *sugarCane  = Block::ByName("sugar_cane");
    Block *waterStill = Block::ByName("water_still");
    Block *waterFlow  = Block::ByName("water_flow");
    Block *grassBlock = Block::ByName("grass_block");
    Block *dirt       = Block::ByName("dirt");
    Block *sand       = Block::ByName("sand");
    Block *redSand    = Block::ByName("red_sand");
    Random random(m_generator->GetChunkSeed(pos, 0xd2a98b26625eee7bULL));

    int attempts = chunk.GetSurfaceBiome(Chunk::s_sizeX / 2, Chunk::s_sizeZ / 2)->GetSugarCaneAttempts() + extraAttempts;
    for (int attempt = 0; attempt < attempts; attempt++)
    {
        int centerX = random.NextInt(0, Chunk::s_sizeX - 1);
        int centerZ = random.NextInt(0, Chunk::s_sizeZ - 1);
        int y       = random.NextInt(0, genDepth - 1);
        for (int i = 0; i < triesPerPatch; i++)
        {
            int x = centerX + random.NextInt(0, 3) - random.NextInt(0, 3);
            int z = centerZ + random.NextInt(0, 3) - random.NextInt(0, 3);
            // We keep a one block border so the water checks never leave the chunk
            if (x < 1 || x > Chunk::s_sizeX - 2 || z < 1 || z > Chunk::s_sizeZ - 2 || y < 1 || Block::ById(chunk.GetBlockId(x, y, z)) != air)
            {
                continue;
            }

            Block *ground = Block::ById(chunk.GetBlockId(x, y - 1, z));
            if (ground != grassBlock && ground != dirt && ground != sand && ground != redSand)
            {
                continue;
            }

            const int offsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            bool water              = false;
            for (int j = 0; j < 4; j++)
            {
                Block *side = Block::ById(chunk.GetBlockId(x + offsets[j][0], y - 1, z + offsets[j][1]));
                water       = water || side == waterStill || side == waterFlow;
            }

            if (!water)
            {
                continue;
            }

            int height = 2 + random.NextInt(0, random.NextInt(0, 2));
            for (int j = 0; j < height && Block::ById(chunk.GetBlockId(x, y + j, z)) == air; j++)
            {
                chunk.SetBlock(x, y + j, z, sugarCane);
            }
        }
    }
}
