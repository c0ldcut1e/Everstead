#include "StoneVariantStage.h"

#include "../block/Block.h"
#include "TerrainGenerator.h"
#include "VeinFeature.h"

StoneVariantStage::StoneVariantStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void StoneVariantStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int veinSize     = 33;
    const int veinsPerType = 10;
    const int maxY         = 80 + TerrainGenerator::s_terrainBase;

    Block *stone                  = Block::ByName("stone");
    Block *variants[]             = {Block::ByName("granite"), Block::ByName("diorite"), Block::ByName("andesite")};
    const uint64_t variantSalts[] = {0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL};
    for (int ownerX = pos.x - 1; ownerX <= pos.x + 1; ownerX++)
    {
        for (int ownerZ = pos.z - 1; ownerZ <= pos.z + 1; ownerZ++)
        {
            ChunkPos owner(ownerX, ownerZ);
            for (int type = 0; type < 3; type++)
            {
                Random random(m_generator->GetChunkSeed(owner, variantSalts[type]));
                for (int vein = 0; vein < veinsPerType; vein++)
                {
                    int x = ownerX * Chunk::s_sizeX + random.NextInt(0, Chunk::s_sizeX - 1);
                    int z = ownerZ * Chunk::s_sizeZ + random.NextInt(0, Chunk::s_sizeZ - 1);
                    int y = random.NextInt(0, maxY - 1);
                    VeinFeature::Place(chunk, pos, random, x, y, z, variants[type], stone, veinSize);
                }
            }
        }
    }
}
