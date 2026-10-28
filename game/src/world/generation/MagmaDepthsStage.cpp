#include "MagmaDepthsStage.h"

#include <algorithm>
#include <iterator>
#include <string>

#include "../../utils/Random.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"
#include "VeinFeature.h"

MagmaDepthsStage::MagmaDepthsStage(TerrainGenerator *generator)
{
    m_generator   = generator;
    m_magmaDepths = Biome::ByName("magma_depths");
}

void MagmaDepthsStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int magmaChance   = 4;
    const int pillarChance  = 24;
    const int maxPillarSize = 12;

    Block *basalt          = Block::ByName("basalt");
    Block *magma           = Block::ByName("magma");
    const char *oreNames[] = {"coal", "iron", "gold", "diamond", "emerald", "lapis", "redstone"};
    Block *stoneOres[std::size(oreNames)];
    Block *basaltOres[std::size(oreNames)];
    for (size_t i = 0; i < std::size(oreNames); i++)
    {
        stoneOres[i]  = Block::ByName(std::string(oreNames[i]) + "_ore");
        basaltOres[i] = Block::ByName(std::string("basalt_") + oreNames[i] + "_ore");
    }

    Random random(m_generator->GetChunkSeed(pos, 0x9b05688c2b3e6c1fULL));
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = 1; y < Chunk::s_sizeY - 1; y++)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_magmaDepths)
                {
                    continue;
                }

                Block *block = Block::ById(chunk.GetBlockId(x, y, z));
                Block **ore  = std::find(stoneOres, stoneOres + std::size(oreNames), block);
                if (ore != stoneOres + std::size(oreNames))
                {
                    chunk.SetBlock(x, y, z, basaltOres[ore - stoneOres]);
                    continue;
                }

                if (!TerrainGenerator::IsStone(block))
                {
                    continue;
                }

                bool floor = chunk.GetBlockId(x, y + 1, z) == 0;
                chunk.SetBlock(x, y, z, floor && random.NextInt(0, magmaChance - 1) == 0 ? magma : basalt);
                chunk.SetBlockData(x, y, z, 0);
                if (!floor || random.NextInt(0, pillarChance - 1) != 0)
                {
                    continue;
                }

                int size = random.NextInt(2, maxPillarSize);
                for (int dy = 1; dy <= size && y + dy < Chunk::s_sizeY - 1 && chunk.GetBlockId(x, y + dy, z) == 0; dy++)
                {
                    chunk.SetBlock(x, y + dy, z, basalt);
                }
            }
        }
    }

    Block *quartzOre = Block::ByName("basalt_quartz_ore");
    for (int ownerX = pos.x - 1; ownerX <= pos.x + 1; ownerX++)
    {
        for (int ownerZ = pos.z - 1; ownerZ <= pos.z + 1; ownerZ++)
        {
            Random quartzRandom(m_generator->GetChunkSeed(ChunkPos(ownerX, ownerZ), 0x6a09e667f3bcc908ULL));
            for (int vein = 0; vein < 16; vein++)
            {
                int x = ownerX * Chunk::s_sizeX + quartzRandom.NextInt(0, Chunk::s_sizeX - 1);
                int y = quartzRandom.NextInt(10, 117);
                int z = ownerZ * Chunk::s_sizeZ + quartzRandom.NextInt(0, Chunk::s_sizeZ - 1);
                VeinFeature::Place(chunk, pos, quartzRandom, x, y, z, quartzOre, basalt, 14);
            }
        }
    }
}
