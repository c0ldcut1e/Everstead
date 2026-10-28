#include "DarkOakTreeFeature.h"

#include <cstdlib>

#include "../block/Block.h"
#include "../block/LeavesBlock.h"

DarkOakTreeFeature::DarkOakTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("dark_oak_log");
    m_leaves     = Block::ByName("dark_oak_leaves");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
    m_coarseDirt = Block::ByName("coarse_dirt");
    m_podzol     = Block::ByName("podzol");
}

bool DarkOakTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void DarkOakTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
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

bool DarkOakTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    int height = random.NextInt(0, 2) + random.NextInt(0, 1) + 6;
    if (y < 1 || y + height + 3 > Chunk::s_sizeY || !IsInside(x, y, z) || !IsInside(x + 1, y, z + 1))
    {
        return false;
    }

    for (int dx = 0; dx <= 1; dx++)
    {
        for (int dz = 0; dz <= 1; dz++)
        {
            Block *ground = Block::ById(chunk.GetBlockId(x + dx, y - 1, z + dz));
            if (ground != m_grassBlock && ground != m_dirt && ground != m_coarseDirt && ground != m_podzol)
            {
                return false;
            }
        }
    }

    Grow(chunk, random, x, y, z, height);
    return true;
}

void DarkOakTreeFeature::PlaceGenerated(Chunk &chunk, Random &random, int x, int y, int z) const
{
    int height = random.NextInt(0, 2) + random.NextInt(0, 1) + 6;
    if (y < 1 || y + height + 3 > Chunk::s_sizeY)
    {
        return;
    }

    Grow(chunk, random, x, y, z, height);
}

void DarkOakTreeFeature::Grow(Chunk &chunk, Random &random, int x, int y, int z, int height) const
{
    static const int dirX[4] = {0, 1, 0, -1};
    static const int dirZ[4] = {1, 0, -1, 0};

    for (int dx = 0; dx <= 1; dx++)
    {
        for (int dz = 0; dz <= 1; dz++)
        {
            if (IsInside(x + dx, y - 1, z + dz))
            {
                chunk.SetBlock(x + dx, y - 1, z + dz, m_dirt);
            }
        }
    }

    int dir        = random.NextInt(0, 3);
    int bendStart  = height - random.NextInt(0, 3);
    int bendLength = 2 - random.NextInt(0, 2);
    int topX       = x;
    int topZ       = z;
    int top        = y + height - 1;
    for (int i = 0; i < height; i++)
    {
        if (i >= bendStart && bendLength > 0)
        {
            topX += dirX[dir];
            topZ += dirZ[dir];
            bendLength--;
        }

        SetIfReplaceable(chunk, topX, y + i, topZ, m_log);
        SetIfReplaceable(chunk, topX + 1, y + i, topZ, m_log);
        SetIfReplaceable(chunk, topX, y + i, topZ + 1, m_log);
        SetIfReplaceable(chunk, topX + 1, y + i, topZ + 1, m_log);
    }

    for (int dx = -2; dx <= 0; dx++)
    {
        for (int dz = -2; dz <= 0; dz++)
        {
            SetIfReplaceable(chunk, topX + dx, top - 1, topZ + dz, m_leaves);
            SetIfReplaceable(chunk, topX + 1 - dx, top - 1, topZ + dz, m_leaves);
            SetIfReplaceable(chunk, topX + dx, top - 1, topZ + 1 - dz, m_leaves);
            SetIfReplaceable(chunk, topX + 1 - dx, top - 1, topZ + 1 - dz, m_leaves);
            if ((dx > -2 || dz > -1) && (dx != -1 || dz != -2))
            {
                SetIfReplaceable(chunk, topX + dx, top + 1, topZ + dz, m_leaves);
                SetIfReplaceable(chunk, topX + 1 - dx, top + 1, topZ + dz, m_leaves);
                SetIfReplaceable(chunk, topX + dx, top + 1, topZ + 1 - dz, m_leaves);
                SetIfReplaceable(chunk, topX + 1 - dx, top + 1, topZ + 1 - dz, m_leaves);
            }
        }
    }

    if (random.NextInt(0, 1) == 0)
    {
        SetIfReplaceable(chunk, topX, top + 2, topZ, m_leaves);
        SetIfReplaceable(chunk, topX + 1, top + 2, topZ, m_leaves);
        SetIfReplaceable(chunk, topX + 1, top + 2, topZ + 1, m_leaves);
        SetIfReplaceable(chunk, topX, top + 2, topZ + 1, m_leaves);
    }

    for (int dx = -3; dx <= 4; dx++)
    {
        for (int dz = -3; dz <= 4; dz++)
        {
            bool corner = (dx == -3 || dx == 4) && (dz == -3 || dz == 4);
            if (!corner && (std::abs(dx) < 3 || std::abs(dz) < 3))
            {
                SetIfReplaceable(chunk, topX + dx, top, topZ + dz, m_leaves);
            }
        }
    }

    for (int dx = -1; dx <= 2; dx++)
    {
        for (int dz = -1; dz <= 2; dz++)
        {
            if ((dx >= 0 && dx <= 1 && dz >= 0 && dz <= 1) || random.NextInt(0, 2) != 0)
            {
                continue;
            }

            int length = random.NextInt(2, 4);
            for (int i = 0; i < length; i++)
            {
                SetIfReplaceable(chunk, topX + dx, top - i - 1, topZ + dz, m_log);
            }

            for (int lx = -1; lx <= 1; lx++)
            {
                for (int lz = -1; lz <= 1; lz++)
                {
                    SetIfReplaceable(chunk, topX + dx + lx, top, topZ + dz + lz, m_leaves);
                }
            }

            for (int lx = -2; lx <= 2; lx++)
            {
                for (int lz = -2; lz <= 2; lz++)
                {
                    if (std::abs(lx) != 2 || std::abs(lz) != 2)
                    {
                        SetIfReplaceable(chunk, topX + dx + lx, top - 1, topZ + dz + lz, m_leaves);
                    }
                }
            }
        }
    }
}
