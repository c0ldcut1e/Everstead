#include "FractureStage.h"

#include "../block/Block.h"
#include "TerrainGenerator.h"

FractureStage::FractureStage(TerrainGenerator *generator)
{
    m_generator = generator;
    m_air       = Block::ByName("air");
    m_bedrock   = Block::ByName("bedrock");
    m_frame     = Block::ByName("broken_fracture_frame");
    m_socket    = Block::ByName("resonant_socket");
}

void FractureStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    Random random(m_generator->GetChunkSeed(pos, 0x9b05688c2b3e6c1fULL));
    if (random.NextInt(0, 1999) != 0)
    {
        return;
    }

    const int offsets[5][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int x                   = random.NextInt(1, Chunk::s_sizeX - 2);
    int y                   = random.NextInt(1, 4);
    int z                   = random.NextInt(1, Chunk::s_sizeZ - 2);
    int socketArm           = random.NextInt(1, 4);
    for (int i = 0; i < 5; i++)
    {
        int xx = x + offsets[i][0];
        int zz = z + offsets[i][1];
        for (int yy = 0; yy < y; yy++)
        {
            chunk.SetBlock(xx, yy, zz, m_bedrock);
            chunk.SetBlockData(xx, yy, zz, 0);
        }

        chunk.SetBlock(xx, y, zz, i == 0 ? m_air : i == socketArm ? m_socket : m_frame);
        chunk.SetBlockData(xx, y, zz, 0);
    }
}
