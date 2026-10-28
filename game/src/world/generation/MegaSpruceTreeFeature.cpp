#include "MegaSpruceTreeFeature.h"

#include <cmath>
#include <cstdlib>

#include "../block/Block.h"
#include "../block/LeavesBlock.h"

MegaSpruceTreeFeature::MegaSpruceTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("spruce_log");
    m_leaves     = Block::ByName("spruce_leaves");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
    m_coarseDirt = Block::ByName("coarse_dirt");
    m_podzol     = Block::ByName("podzol");
}

bool MegaSpruceTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void MegaSpruceTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
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

void MegaSpruceTreeFeature::PlaceLeavesLayer(Chunk &chunk, int x, int y, int z, int width) const
{
    int radiusSquared = width * width;
    for (int dx = -width; dx <= width + 1; dx++)
    {
        for (int dz = -width; dz <= width + 1; dz++)
        {
            int dx1 = dx - 1;
            int dz1 = dz - 1;
            if (dx * dx + dz * dz <= radiusSquared || dx1 * dx1 + dz1 * dz1 <= radiusSquared || dx * dx + dz1 * dz1 <= radiusSquared ||
                dx1 * dx1 + dz * dz <= radiusSquared)
            {
                SetIfReplaceable(chunk, x + dx, y, z + dz, m_leaves);
            }
        }
    }
}

bool MegaSpruceTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z, bool tallCrown) const
{
    int height = random.NextInt(0, 2) + 13 + random.NextInt(0, 14);
    if (y < 1 || y + height + 1 > Chunk::s_sizeY || !IsInside(x, y, z) || !IsInside(x + 1, y, z + 1))
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

    Grow(chunk, random, x, y, z, height, tallCrown);
    return true;
}

void MegaSpruceTreeFeature::PlaceGenerated(Chunk &chunk, Random &random, int x, int y, int z, bool tallCrown) const
{
    int height = random.NextInt(0, 2) + 13 + random.NextInt(0, 14);
    if (y < 1 || y + height + 1 > Chunk::s_sizeY)
    {
        return;
    }

    Grow(chunk, random, x, y, z, height, tallCrown);
    PlacePodzolCircle(chunk, x - 1, y, z - 1);
    PlacePodzolCircle(chunk, x + 2, y, z - 1);
    PlacePodzolCircle(chunk, x - 1, y, z + 2);
    PlacePodzolCircle(chunk, x + 2, y, z + 2);
    for (int i = 0; i < 5; i++)
    {
        int cell = random.NextInt(0, 63);
        int dx   = cell % 8;
        int dz   = cell / 8;
        if (dx == 0 || dx == 7 || dz == 0 || dz == 7)
        {
            PlacePodzolCircle(chunk, x - 3 + dx, y, z - 3 + dz);
        }
    }
}

void MegaSpruceTreeFeature::Grow(Chunk &chunk, Random &random, int x, int y, int z, int height, bool tallCrown) const
{
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

    int top         = y + height;
    int crownHeight = random.NextInt(0, 4) + (tallCrown ? 13 : 3);
    int lastWidth   = 0;
    for (int yy = top - crownHeight; yy <= top; yy++)
    {
        int fromTop = top - yy;
        int width   = (int) std::floor((float) fromTop / (float) crownHeight * 3.5f);
        PlaceLeavesLayer(chunk, x, yy, z, width + (fromTop > 0 && width == lastWidth && (yy & 1) == 0 ? 1 : 0));
        lastWidth = width;
    }

    for (int i = 0; i < height; i++)
    {
        SetIfReplaceable(chunk, x, y + i, z, m_log);
        if (i < height - 1)
        {
            SetIfReplaceable(chunk, x + 1, y + i, z, m_log);
            SetIfReplaceable(chunk, x + 1, y + i, z + 1, m_log);
            SetIfReplaceable(chunk, x, y + i, z + 1, m_log);
        }
    }
}

void MegaSpruceTreeFeature::PlacePodzolCircle(Chunk &chunk, int x, int y, int z) const
{
    for (int dx = -2; dx <= 2; dx++)
    {
        for (int dz = -2; dz <= 2; dz++)
        {
            if (std::abs(dx) != 2 || std::abs(dz) != 2)
            {
                PlacePodzolAt(chunk, x + dx, y, z + dz);
            }
        }
    }
}

void MegaSpruceTreeFeature::PlacePodzolAt(Chunk &chunk, int x, int y, int z) const
{
    for (int dy = 2; dy >= -3; dy--)
    {
        if (!IsInside(x, y + dy, z))
        {
            continue;
        }

        Block *block = Block::ById(chunk.GetBlockId(x, y + dy, z));
        if (block == m_grassBlock || block == m_dirt)
        {
            chunk.SetBlock(x, y + dy, z, m_podzol);
            return;
        }

        if (block != m_air && dy < 0)
        {
            return;
        }
    }
}
