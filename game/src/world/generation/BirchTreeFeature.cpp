#include "BirchTreeFeature.h"

#include <cstdlib>

#include "../block/Block.h"
#include "../block/LeavesBlock.h"

BirchTreeFeature::BirchTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("birch_log");
    m_leaves     = Block::ByName("birch_leaves");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

bool BirchTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void BirchTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
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

void BirchTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    int treeHeight = random.NextInt(0, 2) + 5;
    if (y < 1 || y + treeHeight + 1 > Chunk::s_sizeY)
    {
        return;
    }

    if (IsInside(x, y - 1, z) && Block::ById(chunk.GetBlockId(x, y - 1, z)) == m_grassBlock)
    {
        chunk.SetBlock(x, y - 1, z, m_dirt);
    }

    for (int yy = y - 3 + treeHeight; yy <= y + treeHeight; yy++)
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
