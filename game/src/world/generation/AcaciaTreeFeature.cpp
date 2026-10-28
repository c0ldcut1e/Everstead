#include "AcaciaTreeFeature.h"

#include <cstdlib>

#include "../block/Block.h"
#include "../block/LeavesBlock.h"

AcaciaTreeFeature::AcaciaTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("acacia_log");
    m_leaves     = Block::ByName("acacia_leaves");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

bool AcaciaTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void AcaciaTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
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

void AcaciaTreeFeature::PlaceCanopy(Chunk &chunk, int x, int y, int z, int radius) const
{
    for (int dx = -radius; dx <= radius; dx++)
    {
        for (int dz = -radius; dz <= radius; dz++)
        {
            if (abs(dx) != radius || abs(dz) != radius)
            {
                SetIfReplaceable(chunk, x + dx, y, z + dz, m_leaves);
            }
        }
    }

    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dz = -1; dz <= 1; dz++)
        {
            SetIfReplaceable(chunk, x + dx, y + 1, z + dz, m_leaves);
        }
    }
}

void AcaciaTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    static const int dirX[4] = {0, 1, 0, -1};
    static const int dirZ[4] = {1, 0, -1, 0};

    int treeHeight = random.NextInt(0, 2) + random.NextInt(0, 2) + 5;
    if (y < 1 || y + treeHeight + 2 > Chunk::s_sizeY)
    {
        return;
    }

    if (IsInside(x, y - 1, z) && Block::ById(chunk.GetBlockId(x, y - 1, z)) == m_grassBlock)
    {
        chunk.SetBlock(x, y - 1, z, m_dirt);
    }

    int dir        = random.NextInt(0, 3);
    int leanHeight = treeHeight - random.NextInt(0, 3) - 1;
    int leanSteps  = 3 - random.NextInt(0, 2);
    int trunkX     = x;
    int trunkZ     = z;
    int topY       = y;
    for (int i = 0; i < treeHeight; i++)
    {
        if (i >= leanHeight && leanSteps > 0)
        {
            trunkX += dirX[dir];
            trunkZ += dirZ[dir];
            leanSteps--;
        }

        topY = y + i;
        SetIfReplaceable(chunk, trunkX, topY, trunkZ, m_log);
    }

    PlaceCanopy(chunk, trunkX, topY, trunkZ, 3);
    SetIfReplaceable(chunk, trunkX + 2, topY + 1, trunkZ, m_leaves);
    SetIfReplaceable(chunk, trunkX - 2, topY + 1, trunkZ, m_leaves);
    SetIfReplaceable(chunk, trunkX, topY + 1, trunkZ + 2, m_leaves);
    SetIfReplaceable(chunk, trunkX, topY + 1, trunkZ - 2, m_leaves);

    int branchDir = random.NextInt(0, 3);
    if (branchDir == dir)
    {
        return;
    }

    int branchStart = leanHeight - random.NextInt(0, 1) - 1;
    int branchSteps = 1 + random.NextInt(0, 2);
    int branchX     = x;
    int branchZ     = z;
    int branchTopY  = -1;
    for (int i = branchStart; i < treeHeight && branchSteps > 0; i++, branchSteps--)
    {
        if (i < 1)
        {
            continue;
        }

        branchX += dirX[branchDir];
        branchZ += dirZ[branchDir];
        branchTopY = y + i;
        SetIfReplaceable(chunk, branchX, branchTopY, branchZ, m_log);
    }

    if (branchTopY > 0)
    {
        PlaceCanopy(chunk, branchX, branchTopY, branchZ, 2);
    }
}
