#include "SpruceTreeFeature.h"

#include <cstdlib>

#include "../block/Block.h"
#include "../block/LeavesBlock.h"

SpruceTreeFeature::SpruceTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("spruce_log");
    m_leaves     = Block::ByName("spruce_leaves");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

bool SpruceTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void SpruceTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
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

void SpruceTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    int treeHeight  = random.NextInt(0, 3) + 6;
    int trunkHeight = 1 + random.NextInt(0, 1);
    int topHeight   = treeHeight - trunkHeight;
    int leafRadius  = 2 + random.NextInt(0, 1);
    if (y < 1 || y + treeHeight + 1 > Chunk::s_sizeY)
    {
        return;
    }

    if (IsInside(x, y - 1, z) && Block::ById(chunk.GetBlockId(x, y - 1, z)) == m_grassBlock)
    {
        chunk.SetBlock(x, y - 1, z, m_dirt);
    }

    int currentRadius = random.NextInt(0, 1);
    int maxRadius     = 1;
    int minRadius     = 0;
    for (int heightPos = 0; heightPos <= topHeight; heightPos++)
    {
        int yy = y + treeHeight - heightPos;
        for (int xx = x - currentRadius; xx <= x + currentRadius; xx++)
        {
            for (int zz = z - currentRadius; zz <= z + currentRadius; zz++)
            {
                if (currentRadius > 0 && abs(xx - x) == currentRadius && abs(zz - z) == currentRadius)
                {
                    continue;
                }

                SetIfReplaceable(chunk, xx, yy, zz, m_leaves);
            }
        }

        if (currentRadius >= maxRadius)
        {
            currentRadius = minRadius;
            minRadius     = 1;
            maxRadius++;
            if (maxRadius > leafRadius)
            {
                maxRadius = leafRadius;
            }
        }
        else
        {
            currentRadius++;
        }
    }

    int topOffset = random.NextInt(0, 2);
    for (int hh = 0; hh < treeHeight - topOffset; hh++)
    {
        SetIfReplaceable(chunk, x, y + hh, z, m_log);
    }
}
