#include "SpringStage.h"

#include "../block/Block.h"
#include "TerrainGenerator.h"

SpringStage::SpringStage(TerrainGenerator *generator, Block *liquid, bool lava)
{
    m_generator = generator;
    m_liquid    = liquid;
    m_lava      = lava;
}

void SpringStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int waterSpringAttempts = 50;
    const int lavaSpringAttempts  = 20;

    Block *air = Block::ByName("air");

    Random random(m_generator->GetChunkSeed(pos, m_lava ? 0x9fb21c651e98df25ULL : 0x6a09e667f3bcc909ULL));
    int springAttempts = m_lava ? lavaSpringAttempts : waterSpringAttempts;
    for (int attempt = 0; attempt < springAttempts; attempt++)
    {
        int x = random.NextInt(1, Chunk::s_sizeX - 2);
        int z = random.NextInt(1, Chunk::s_sizeZ - 2);
        int y = m_lava ? random.NextInt(0, random.NextInt(0, random.NextInt(0, TerrainGenerator::s_genDepth - 17) + 7) + 7)
                       : random.NextInt(0, random.NextInt(0, TerrainGenerator::s_genDepth - 9) + 7);
        if (y < 1 || y >= TerrainGenerator::s_genDepth - 1)
        {
            continue;
        }

        Block *current = Block::ById(chunk.GetBlockId(x, y, z));
        if (!TerrainGenerator::IsStone(Block::ById(chunk.GetBlockId(x, y + 1, z))) ||
            !TerrainGenerator::IsStone(Block::ById(chunk.GetBlockId(x, y - 1, z))) || (current != air && !TerrainGenerator::IsStone(current)))
        {
            continue;
        }

        int stoneSides          = 0;
        int airSides            = 0;
        const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (int i = 0; i < 4; i++)
        {
            Block *side = Block::ById(chunk.GetBlockId(x + offsets[i][0], y, z + offsets[i][1]));
            stoneSides += TerrainGenerator::IsStone(side) ? 1 : 0;
            airSides += side == air ? 1 : 0;
        }

        if (stoneSides == 3 && airSides == 1)
        {
            chunk.SetBlock(x, y, z, m_liquid);
            chunk.SetBlockData(x, y, z, 0);
        }
    }
}
