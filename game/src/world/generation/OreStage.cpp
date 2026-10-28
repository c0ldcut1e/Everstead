#include "OreStage.h"

#include "../block/Block.h"
#include "TerrainGenerator.h"
#include "VeinFeature.h"

OreStage::OreStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void OreStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const uint64_t oreSalts[] = {0x243f6a8885a308d3ULL, 0x13198a2e03707344ULL, 0xa4093822299f31d0ULL, 0x082efa98ec4e6c89ULL, 0x452821e638d01377ULL};

    Block *stone                       = Block::ByName("stone");
    const std::vector<OreConfig> &ores = m_generator->GetSettings().oreConfigs;
    for (int ownerX = pos.x - 1; ownerX <= pos.x + 1; ownerX++)
    {
        for (int ownerZ = pos.z - 1; ownerZ <= pos.z + 1; ownerZ++)
        {
            ChunkPos owner(ownerX, ownerZ);
            for (size_t type = 0; type < ores.size(); type++)
            {
                const OreConfig &config = ores[type];
                Block *ore              = Block::ByName(config.block);
                Random random(m_generator->GetChunkSeed(owner, type < 5 ? oreSalts[type] : oreSalts[0] + type * 0x9e3779b97f4a7c15ULL));
                for (int vein = 0; vein < config.veinCount; vein++)
                {
                    int x    = ownerX * Chunk::s_sizeX + random.NextInt(0, Chunk::s_sizeX - 1);
                    int z    = ownerZ * Chunk::s_sizeZ + random.NextInt(0, Chunk::s_sizeZ - 1);
                    int span = config.maxY - config.minY;
                    int y    = config.minY +
                               (config.triangular ? random.NextInt(0, span / 2) + random.NextInt(0, span - span / 2) : random.NextInt(0, span));
                    VeinFeature::Place(chunk, pos, random, x, y, z, ore, stone, config.veinSize);
                }
            }
        }
    }

    Block *emeraldOre = Block::ByName("emerald_ore");
    Random random(m_generator->GetChunkSeed(pos, 0x9216d5d98979fb1bULL));
    int count = random.NextInt(3, 8);
    for (int i = 0; i < count; i++)
    {
        int x = random.NextInt(0, Chunk::s_sizeX - 1);
        int y = random.NextInt(4, 31);
        int z = random.NextInt(0, Chunk::s_sizeZ - 1);
        if (m_generator->GetBiomeSource().GetBiome(pos.x * Chunk::s_sizeX + x, pos.z * Chunk::s_sizeZ + z)->HasEmeralds() &&
            Block::ById(chunk.GetBlockId(x, y, z)) == stone)
        {
            chunk.SetBlock(x, y, z, emeraldOre);
        }
    }
}
