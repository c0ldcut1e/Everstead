#include "ClayStage.h"

#include "../block/Block.h"
#include "TerrainGenerator.h"

ClayStage::ClayStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void ClayStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int maxRadius = 3;

    Block *air   = Block::ByName("air");
    Block *water = Block::ByName("water_still");
    Block *dirt  = Block::ByName("dirt");
    Block *clay  = Block::ByName("clay");

    Random random(m_generator->GetChunkSeed(pos, 0x3c6ef372fe94f82bULL));
    int x = random.NextInt(maxRadius, Chunk::s_sizeX - 1 - maxRadius);
    int z = random.NextInt(maxRadius, Chunk::s_sizeZ - 1 - maxRadius);
    int y = Chunk::s_sizeY - 1;
    while (y > 0 && (Block::ById(chunk.GetBlockId(x, y, z)) == air || Block::ById(chunk.GetBlockId(x, y, z)) == water))
    {
        y--;
    }

    if (y >= Chunk::s_sizeY - 2 || Block::ById(chunk.GetBlockId(x, y + 1, z)) != water)
    {
        return;
    }

    int radius = random.NextInt(2, maxRadius);
    for (int dx = -radius; dx <= radius; dx++)
    {
        for (int dz = -radius; dz <= radius; dz++)
        {
            if (dx * dx + dz * dz > radius * radius)
            {
                continue;
            }

            for (int dy = -1; dy <= 1; dy++)
            {
                Block *block = Block::ById(chunk.GetBlockId(x + dx, y + dy, z + dz));
                if (block == dirt || block == clay)
                {
                    chunk.SetBlock(x + dx, y + dy, z + dz, clay);
                }
            }
        }
    }
}
