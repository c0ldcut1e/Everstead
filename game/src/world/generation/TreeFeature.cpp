#include "TreeFeature.h"

#include <cstdlib>

#include "../block/Block.h"
#include "../block/LeavesBlock.h"

TreeFeature::TreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("oak_log");
    m_leaves     = Block::ByName("oak_leaves");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

bool TreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void TreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
{
    if (!IsInside(x, y, z))
    {
        return;
    }

    Block *existing = Block::ById(chunk.GetBlockId(x, y, z));
    if (existing == m_air || existing == m_leaves)
    {
        chunk.SetBlock(x, y, z, block);
        if (block == m_leaves)
        {
            chunk.SetBlockData(x, y, z, ((LeavesBlock *) m_leaves)->GetNaturalData());
        }
    }
}

void TreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    const int baseHeight = 4;
    const int leafLayers = 3;

    int treeHeight = random.NextInt(0, 2) + baseHeight;
    if (y < 1 || y + treeHeight + 1 > Chunk::s_sizeY)
    {
        return;
    }

    if (IsInside(x, y - 1, z) && Block::ById(chunk.GetBlockId(x, y - 1, z)) == m_grassBlock)
    {
        chunk.SetBlock(x, y - 1, z, m_dirt);
    }

    for (int yy = y + treeHeight; yy >= y - leafLayers + treeHeight; yy--)
    {
        int yo   = yy - (y + treeHeight);
        int offs = 1 - yo / 2;
        for (int xx = x - offs; xx <= x + offs; xx++)
        {
            for (int zz = z - offs; zz <= z + offs; zz++)
            {
                if (abs(xx - x) == offs && abs(zz - z) == offs && (random.NextInt(0, 1) == 0 || yo == 0))
                {
                    continue;
                }

                SetIfReplaceable(chunk, xx, yy, zz, m_leaves);
            }
        }
    }

    for (int hh = 0; hh < treeHeight; hh++)
    {
        SetIfReplaceable(chunk, x, y + hh, z, m_log);
    }
}
