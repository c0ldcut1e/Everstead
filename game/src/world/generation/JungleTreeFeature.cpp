#include "JungleTreeFeature.h"

#include <cmath>
#include <cstdlib>
#include <numbers>

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/CocoaBlock.h"
#include "../block/LeavesBlock.h"

JungleTreeFeature::JungleTreeFeature()
{
    m_air        = Block::ByName("air");
    m_log        = Block::ByName("jungle_log");
    m_leaves     = Block::ByName("jungle_leaves");
    m_oakLeaves  = Block::ByName("oak_leaves");
    m_vine       = Block::ByName("vine");
    m_cocoa      = Block::ByName("cocoa");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

bool JungleTreeFeature::IsInside(int x, int y, int z)
{
    return x >= 0 && z >= 0 && x < Chunk::s_sizeX && z < Chunk::s_sizeZ && y >= 0 && y < Chunk::s_sizeY;
}

bool JungleTreeFeature::IsAir(Chunk &chunk, int x, int y, int z) const
{
    return IsInside(x, y, z) && Block::ById(chunk.GetBlockId(x, y, z)) == m_air;
}

void JungleTreeFeature::SetIfReplaceable(Chunk &chunk, int x, int y, int z, Block *block) const
{
    if (!IsInside(x, y, z))
    {
        return;
    }

    Block *existing = Block::ById(chunk.GetBlockId(x, y, z));
    if (existing == m_air || existing == m_leaves || existing == m_oakLeaves || existing == m_vine)
    {
        chunk.SetBlock(x, y, z, block);
    }
}

void JungleTreeFeature::SetLeaves(Chunk &chunk, int x, int y, int z, Block *leaves) const
{
    if (IsAir(chunk, x, y, z) || (IsInside(x, y, z) && Block::ById(chunk.GetBlockId(x, y, z)) == m_vine))
    {
        chunk.SetBlock(x, y, z, leaves);
        chunk.SetBlockData(x, y, z, ((LeavesBlock *) leaves)->GetNaturalData());
    }
}

void JungleTreeFeature::PlaceVine(Chunk &chunk, int x, int y, int z, Direction *direction) const
{
    if (IsAir(chunk, x, y, z))
    {
        chunk.SetBlock(x, y, z, m_vine);
        chunk.SetBlockData(x, y, z, m_vine->GetPlacementData(direction));
    }
}

void JungleTreeFeature::HangVine(Chunk &chunk, int x, int y, int z, Direction *direction) const
{
    const int maxLength = 4;

    for (int i = 0; i < maxLength && IsAir(chunk, x, y - i, z); i++)
    {
        PlaceVine(chunk, x, y - i, z, direction);
    }
}

// We roll for every side before checking the chunk so every chunk the tree crosses draws the same numbers
void JungleTreeFeature::HangLeafVines(Chunk &chunk, Random &random, int minX, int minY, int minZ, int maxX, int maxY, int maxZ) const
{
    Direction *sides[] = {Direction::north, Direction::east, Direction::south, Direction::west};
    for (int yy = maxY; yy >= minY; yy--)
    {
        for (int xx = minX; xx <= maxX; xx++)
        {
            for (int zz = minZ; zz <= maxZ; zz++)
            {
                for (int i = 0; i < 4; i++)
                {
                    bool roll = random.NextInt(0, 3) == 0;
                    if (roll && IsInside(xx, yy, zz) && Block::ById(chunk.GetBlockId(xx, yy, zz)) == m_leaves)
                    {
                        HangVine(chunk, xx + sides[i]->dx, yy, zz + sides[i]->dz, sides[i]);
                    }
                }
            }
        }
    }
}

void JungleTreeFeature::PlaceDirt(Chunk &chunk, int x, int y, int z) const
{
    if (IsInside(x, y, z) && Block::ById(chunk.GetBlockId(x, y, z)) == m_grassBlock)
    {
        chunk.SetBlock(x, y, z, m_dirt);
    }
}

void JungleTreeFeature::Place(Chunk &chunk, Random &random, int x, int y, int z) const
{
    const int baseHeight = 4;
    const int leafLayers = 3;

    int treeHeight = baseHeight + random.NextInt(0, 6);
    if (y < 1 || y + treeHeight + 1 > Chunk::s_sizeY)
    {
        return;
    }

    PlaceDirt(chunk, x, y - 1, z);
    for (int yy = y + treeHeight; yy >= y - leafLayers + treeHeight; yy--)
    {
        int yo   = yy - (y + treeHeight);
        int offs = 1 - yo / 2;
        for (int xx = x - offs; xx <= x + offs; xx++)
        {
            for (int zz = z - offs; zz <= z + offs; zz++)
            {
                bool corner = abs(xx - x) == offs && abs(zz - z) == offs;
                if (corner && (random.NextInt(0, 1) == 0 || yo == 0))
                {
                    continue;
                }

                SetLeaves(chunk, xx, yy, zz, m_leaves);
            }
        }
    }

    Direction *sides[] = {Direction::north, Direction::east, Direction::south, Direction::west};
    for (int hh = 0; hh < treeHeight; hh++)
    {
        SetIfReplaceable(chunk, x, y + hh, z, m_log);
        if (hh == 0)
        {
            continue;
        }

        for (int i = 0; i < 4; i++)
        {
            if (random.NextInt(0, 2) > 0)
            {
                PlaceVine(chunk, x + sides[i]->dx, y + hh, z + sides[i]->dz, sides[i]);
            }
        }
    }

    HangLeafVines(chunk, random, x - 2, y - leafLayers + treeHeight, z - 2, x + 2, y + treeHeight, z + 2);

    if (treeHeight <= 5 || random.NextInt(0, 4) != 0)
    {
        return;
    }

    CocoaBlock *cocoa = (CocoaBlock *) m_cocoa;
    for (int level = 0; level < 2; level++)
    {
        for (int i = 0; i < 4; i++)
        {
            int age = random.NextInt(0, 2);
            if (random.NextInt(0, 3 - level) != 0)
            {
                continue;
            }

            int cx = x + sides[i]->dx;
            int cy = y + treeHeight - 5 + level;
            int cz = z + sides[i]->dz;
            if (IsAir(chunk, cx, cy, cz) || (IsInside(cx, cy, cz) && Block::ById(chunk.GetBlockId(cx, cy, cz)) == m_vine))
            {
                chunk.SetBlock(cx, cy, cz, m_cocoa);
                chunk.SetBlockData(cx, cy, cz, cocoa->WithAge(m_cocoa->GetPlacementData(sides[i]), age));
            }
        }
    }
}

bool JungleTreeFeature::PlaceMega(Chunk &chunk, Random &random, int x, int y, int z) const
{
    const int baseHeight = 10;
    const int crownWidth = 2;

    int treeHeight = baseHeight + random.NextInt(0, 19);
    if (y < 1 || y + treeHeight + 2 > Chunk::s_sizeY)
    {
        return false;
    }

    for (int dx = 0; dx <= 1; dx++)
    {
        for (int dz = 0; dz <= 1; dz++)
        {
            PlaceDirt(chunk, x + dx, y - 1, z + dz);
        }
    }

    int top = y + treeHeight;
    for (int layer = -2; layer <= 0; layer++)
    {
        int width  = crownWidth + 1 - layer;
        int radius = width * width;
        for (int i = -width; i <= width + 1; i++)
        {
            for (int j = -width; j <= width + 1; j++)
            {
                int k = i - 1;
                int l = j - 1;
                if (i * i + j * j <= radius || k * k + l * l <= radius || i * i + l * l <= radius || k * k + j * j <= radius)
                {
                    SetLeaves(chunk, x + i, top + layer, z + j, m_leaves);
                }
            }
        }
    }

    for (int branchY = top - 2 - random.NextInt(0, 3); branchY > y + treeHeight / 2; branchY -= 2 + random.NextInt(0, 3))
    {
        float angle   = random.NextFloat() * 2.0f * std::numbers::pi_v<float>;
        int branchX   = x;
        int branchZ   = z;
        for (int i = 0; i < 5; i++)
        {
            branchX = x + (int) (1.5f + std::cos(angle) * (float) i);
            branchZ = z + (int) (1.5f + std::sin(angle) * (float) i);
            SetIfReplaceable(chunk, branchX, branchY - 3 + i / 2, branchZ, m_log);
        }

        int leafLayers = 1 + random.NextInt(0, 1);
        for (int leafY = branchY - leafLayers; leafY <= branchY; leafY++)
        {
            int radius = 1 - (leafY - branchY);
            for (int dx = -radius; dx <= radius; dx++)
            {
                for (int dz = -radius; dz <= radius; dz++)
                {
                    if (dx * dx + dz * dz <= radius * radius)
                    {
                        SetLeaves(chunk, branchX + dx, leafY, branchZ + dz, m_leaves);
                    }
                }
            }
        }
    }

    for (int hh = 0; hh < treeHeight; hh++)
    {
        for (int dx = 0; dx <= 1; dx++)
        {
            for (int dz = 0; dz <= 1; dz++)
            {
                SetIfReplaceable(chunk, x + dx, y + hh, z + dz, m_log);
            }
        }

        if (hh == 0)
        {
            continue;
        }

        bool westRoll  = random.NextInt(0, 2) > 0;
        bool northRoll = random.NextInt(0, 2) > 0;
        bool eastRoll  = random.NextInt(0, 2) > 0;
        bool southRoll = random.NextInt(0, 2) > 0;
        if (westRoll)
        {
            PlaceVine(chunk, x - 1, y + hh, z + (hh & 1), Direction::west);
        }

        if (northRoll)
        {
            PlaceVine(chunk, x + (hh & 1), y + hh, z - 1, Direction::north);
        }

        if (eastRoll)
        {
            PlaceVine(chunk, x + 2, y + hh, z + (hh & 1), Direction::east);
        }

        if (southRoll)
        {
            PlaceVine(chunk, x + (hh & 1), y + hh, z + 2, Direction::south);
        }
    }

    HangLeafVines(chunk, random, x - 5, y + treeHeight / 2 - 2, z - 5, x + 6, top, z + 6);
    return true;
}

void JungleTreeFeature::PlaceBush(Chunk &chunk, Random &random, int x, int y, int z) const
{
    if (y < 1 || y + 3 > Chunk::s_sizeY)
    {
        return;
    }

    PlaceDirt(chunk, x, y - 1, z);
    for (int yy = y; yy <= y + 2; yy++)
    {
        int radius = 2 - (yy - y);
        for (int xx = x - radius; xx <= x + radius; xx++)
        {
            for (int zz = z - radius; zz <= z + radius; zz++)
            {
                bool corner = abs(xx - x) == radius && abs(zz - z) == radius;
                if (corner && random.NextInt(0, 1) == 0)
                {
                    continue;
                }

                SetLeaves(chunk, xx, yy, zz, m_oakLeaves);
            }
        }
    }

    SetIfReplaceable(chunk, x, y, z, m_log);
}
