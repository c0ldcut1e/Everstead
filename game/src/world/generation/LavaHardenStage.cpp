#include "LavaHardenStage.h"

#include "../block/Block.h"

LavaHardenStage::LavaHardenStage()
{
    m_obsidian    = Block::ByName("obsidian");
    m_cobblestone = Block::ByName("cobblestone");
}

// Generated liquids never tick, so we apply the same hardening rule LiquidBlock uses when water reaches lava
void LavaHardenStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    (void) pos;
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        if (chunk.GetSection(section).IsEmpty())
        {
            continue;
        }

        for (int y = section * ChunkSection::s_size; y < (section + 1) * ChunkSection::s_size; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int x = 0; x < Chunk::s_sizeX; x++)
                {
                    if (!Block::ById(chunk.GetBlockId(x, y, z))->IsLava())
                    {
                        continue;
                    }

                    if (IsWater(chunk, x - 1, y, z) || IsWater(chunk, x + 1, y, z) || IsWater(chunk, x, y, z - 1) || IsWater(chunk, x, y, z + 1) ||
                        IsWater(chunk, x, y + 1, z))
                    {
                        chunk.SetBlock(x, y, z, chunk.GetBlockData(x, y, z) == 0 ? m_obsidian : m_cobblestone);
                        chunk.SetBlockData(x, y, z, 0);
                    }
                }
            }
        }
    }
}

bool LavaHardenStage::IsWater(const Chunk &chunk, int x, int y, int z) const
{
    if (x < 0 || z < 0 || x >= Chunk::s_sizeX || z >= Chunk::s_sizeZ || y >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *block = Block::ById(chunk.GetBlockId(x, y, z));
    return block->IsLiquid() && !block->IsLava();
}
