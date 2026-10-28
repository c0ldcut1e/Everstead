#include "SnowStage.h"

#include "../biome/Biome.h"
#include "../block/Block.h"
#include "../block/LeavesBlock.h"
#include "../block/SnowLayerBlock.h"
#include "TerrainGenerator.h"

SnowStage::SnowStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void SnowStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    SnowLayerBlock *snow = (SnowLayerBlock *) Block::ByName("snow");
    Block *ice           = Block::ByName("ice");
    Block *air           = Block::ByName("air");
    Block *water         = Block::ByName("water_still");

    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            Biome *biome = chunk.GetSurfaceBiome(x, z);
            bool covered = false;
            for (int y = Chunk::s_sizeY - 2; y >= 1; y--)
            {
                Block *block = Block::ById(chunk.GetBlockId(x, y, z));
                if (block == air || (covered && LeavesBlock::IsLeaves(block)))
                {
                    continue;
                }

                if (!biome->IsSnowyAt(y + 1))
                {
                    break;
                }

                if (block == water && chunk.GetBlockData(x, y, z) == 0)
                {
                    chunk.SetBlock(x, y, z, ice);
                }
                else if (block->IsSolid() && !block->IsLiquid() && block->GetRenderShape() == Block::eRenderShape_CUBE &&
                         Block::ById(chunk.GetBlockId(x, y + 1, z)) == air)
                {
                    chunk.SetBlock(x, y + 1, z, snow);
                    if (!covered && !LeavesBlock::IsLeaves(block))
                    {
                        int layers = snow->GetDriftLayers(m_generator->GetSeed(), pos.x * Chunk::s_sizeX + x, pos.z * Chunk::s_sizeZ + z);
                        chunk.SetBlockData(x, y + 1, z, snow->WithLayers(layers));
                    }
                }

                if (!LeavesBlock::IsLeaves(block))
                {
                    break;
                }

                covered = true;
            }
        }
    }
}
