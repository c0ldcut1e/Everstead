#include "ForestRockStage.h"

#include "../../utils/Random.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"

ForestRockStage::ForestRockStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void ForestRockStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    Block *mossyCobblestone = Block::ByName("mossy_cobblestone");
    int chunkX              = pos.x * Chunk::s_sizeX;
    int chunkZ              = pos.z * Chunk::s_sizeZ;
    for (int ownerX = -1; ownerX <= 1; ownerX++)
    {
        for (int ownerZ = -1; ownerZ <= 1; ownerZ++)
        {
            ChunkPos owner(pos.x + ownerX, pos.z + ownerZ);
            Random random(m_generator->GetChunkSeed(owner, 0x2545f4914f6cdd1dULL));
            int rocks = random.NextInt(0, 2);
            for (int rock = 0; rock < rocks; rock++)
            {
                int x         = owner.x * Chunk::s_sizeX + random.NextInt(0, Chunk::s_sizeX - 1);
                int z         = owner.z * Chunk::s_sizeZ + random.NextInt(0, Chunk::s_sizeZ - 1);
                uint64_t seed = random.NextUInt64();
                int y         = m_generator->GetTerrainHeight(x, z);
                if (y < TerrainGenerator::s_seaLevel || !m_generator->GetBiomeSource().GetBiome(x, z)->HasForestRocks())
                {
                    continue;
                }

                Random rockRandom(seed);
                for (int blob = 0; blob < 3; blob++)
                {
                    int radiusX  = rockRandom.NextInt(0, 1);
                    int radiusY  = rockRandom.NextInt(0, 1);
                    int radiusZ  = rockRandom.NextInt(0, 1);
                    float radius = (float) (radiusX + radiusY + radiusZ) * 0.333f + 0.5f;
                    for (int dx = -radiusX; dx <= radiusX; dx++)
                    {
                        for (int dy = -radiusY; dy <= radiusY; dy++)
                        {
                            for (int dz = -radiusZ; dz <= radiusZ; dz++)
                            {
                                int localX = x + dx - chunkX;
                                int localZ = z + dz - chunkZ;
                                if ((float) (dx * dx + dy * dy + dz * dz) <= radius * radius && localX >= 0 && localZ >= 0 &&
                                    localX < Chunk::s_sizeX && localZ < Chunk::s_sizeZ && y + dy > 0 && y + dy < Chunk::s_sizeY)
                                {
                                    chunk.SetBlock(localX, y + dy, localZ, mossyCobblestone);
                                }
                            }
                        }
                    }

                    x += rockRandom.NextInt(0, 1) - 1;
                    y -= rockRandom.NextInt(0, 1);
                    z += rockRandom.NextInt(0, 1) - 1;
                }
            }
        }
    }
}
