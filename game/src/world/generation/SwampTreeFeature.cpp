#include "SwampTreeFeature.h"

#include <cstdlib>

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/LeavesBlock.h"

SwampTreeFeature::SwampTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("oak_log");
    m_leaves     = Block::ByName("oak_leaves");
    m_vine       = Block::ByName("vine");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

bool SwampTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void SwampTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
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

void SwampTreeFeature::HangVine(Chunk &chunk, int x, int y, int z, Direction *direction) const
{
    const int maxLength = 4;

    uint16_t data = m_vine->GetPlacementData(direction);
    for (int i = 0; i < maxLength && IsInside(x, y - i, z) && Block::ById(chunk.GetBlockId(x, y - i, z)) == m_air; i++)
    {
        chunk.SetBlock(x, y - i, z, m_vine);
        chunk.SetBlockData(x, y - i, z, data);
    }
}

void SwampTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    const int baseHeight = 5;
    const int leafLayers = 4;

    int treeHeight = random.NextInt(0, 3) + baseHeight;
    if (y < 1 || y + treeHeight + 1 > Chunk::s_sizeY)
    {
        return;
    }

    if (IsInside(x, y - 1, z) && Block::ById(chunk.GetBlockId(x, y - 1, z)) == m_grassBlock)
    {
        chunk.SetBlock(x, y - 1, z, m_dirt);
    }

    for (int yy = y + treeHeight; yy >= y - leafLayers + 1 + treeHeight; yy--)
    {
        int yo   = yy - (y + treeHeight);
        int offs = 2 - yo / 2;
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

    Direction *sides[] = {Direction::north, Direction::east, Direction::south, Direction::west};
    for (int yy = y + treeHeight; yy >= y - leafLayers + 1 + treeHeight; yy--)
    {
        int offs = 2 - (yy - (y + treeHeight)) / 2;
        for (int xx = x - offs; xx <= x + offs; xx++)
        {
            for (int zz = z - offs; zz <= z + offs; zz++)
            {
                if (!IsInside(xx, yy, zz) || Block::ById(chunk.GetBlockId(xx, yy, zz)) != m_leaves)
                {
                    continue;
                }

                for (int i = 0; i < 4; i++)
                {
                    if (random.NextInt(0, 3) == 0)
                    {
                        HangVine(chunk, xx + sides[i]->dx, yy, zz + sides[i]->dz, sides[i]);
                    }
                }
            }
        }
    }
}
