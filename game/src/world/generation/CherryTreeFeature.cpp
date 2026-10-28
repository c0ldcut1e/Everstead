#include "CherryTreeFeature.h"

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/LeavesBlock.h"

CherryTreeFeature::CherryTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("cherry_log");
    m_leaves     = Block::ByName("cherry_leaves");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

bool CherryTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

void CherryTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block, uint16_t data) const
{
    if (!IsInside(x, y, z))
    {
        return;
    }

    Block *existing = Block::ById(chunk.GetBlockId(x, y, z));
    if (existing == m_air || existing == m_leaves)
    {
        chunk.SetBlock(x, y, z, block);
        chunk.SetBlockData(x, y, z, block == m_leaves ? ((LeavesBlock *) m_leaves)->GetNaturalData() : data);
    }
}

// Leaves decay more than 6 steps from a log, so every layer and strand has to stay within that
void CherryTreeFeature::PlaceCanopy(Chunk &chunk, Random &random, int x, int y, int z) const
{
    static const int radii[5] = {3, 4, 4, 3, 2};

    SetIfReplaceable(chunk, x, y, z, m_log);
    for (int dy = -2; dy <= 2; dy++)
    {
        int radius = radii[dy + 2];
        for (int dx = -radius; dx <= radius; dx++)
        {
            for (int dz = -radius; dz <= radius; dz++)
            {
                int distance = dx * dx + dz * dz;
                if (distance <= radius * radius - 2 || (distance <= radius * radius && random.NextInt(0, 2) != 0))
                {
                    SetIfReplaceable(chunk, x + dx, y + dy, z + dz, m_leaves);
                }
            }
        }
    }

    for (int dx = -2; dx <= 2; dx++)
    {
        for (int dz = -2; dz <= 2; dz++)
        {
            if (dx * dx + dz * dz > 4 || random.NextInt(0, 3) != 0)
            {
                continue;
            }

            int length = random.NextInt(1, 2);
            for (int i = 1; i <= length; i++)
            {
                SetIfReplaceable(chunk, x + dx, y - 2 - i, z + dz, m_leaves);
            }
        }
    }
}

void CherryTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    static const int dirX[4] = {0, 1, 0, -1};
    static const int dirZ[4] = {1, 0, -1, 0};

    int trunkHeight = random.NextInt(0, 2) + 5;
    if (y < 1 || y + trunkHeight + 6 > Chunk::s_sizeY)
    {
        return;
    }

    if (IsInside(x, y - 1, z) && Block::ById(chunk.GetBlockId(x, y - 1, z)) == m_grassBlock)
    {
        chunk.SetBlock(x, y - 1, z, m_dirt);
    }

    for (int i = 0; i < trunkHeight; i++)
    {
        SetIfReplaceable(chunk, x, y + i, z, m_log);
    }

    int branchCount = random.NextInt(0, 3) == 0 ? 1 : 2;
    int firstDir    = random.NextInt(0, 3);
    for (int branch = 0; branch < branchCount; branch++)
    {
        int dir     = (firstDir + branch * 2) % 4;
        int branchX = x;
        int branchY = y + trunkHeight - 2 - random.NextInt(0, 1);
        int branchZ = z;
        int steps   = random.NextInt(2, 4);
        for (int i = 0; i < steps; i++)
        {
            branchX += dirX[dir];
            branchZ += dirZ[dir];
            SetIfReplaceable(chunk, branchX, branchY, branchZ, m_log, m_log->GetPlacementData(dirX[dir] != 0 ? Direction::east : Direction::south));
        }

        int rise = random.NextInt(1, 3);
        for (int i = 1; i <= rise; i++)
        {
            SetIfReplaceable(chunk, branchX, branchY + i, branchZ, m_log);
        }

        PlaceCanopy(chunk, random, branchX, branchY + rise + 1, branchZ);
    }

    if (branchCount == 1)
    {
        PlaceCanopy(chunk, random, x, y + trunkHeight, z);
    }
}
