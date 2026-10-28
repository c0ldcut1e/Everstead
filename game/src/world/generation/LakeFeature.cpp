#include "LakeFeature.h"

#include "../block/Block.h"

LakeFeature::LakeFeature()
{
    m_air        = Block::ByName("air");
    m_stone      = Block::ByName("stone");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
}

int LakeFeature::Index(int x, int y, int z)
{
    return (x * s_width + z) * s_height + y;
}

void LakeFeature::BuildPlan(Random &random, int baseY, Plan *plan) const
{
    plan->active = true;
    plan->baseY  = baseY;
    for (int i = 0; i < s_width * s_width * s_height; i++)
    {
        plan->grid[i] = false;
    }

    int spots = random.NextInt(0, 3) + 4;
    for (int i = 0; i < spots; i++)
    {
        double xr = random.NextDouble() * 6.0 + 3.0;
        double yr = random.NextDouble() * 4.0 + 2.0;
        double zr = random.NextDouble() * 6.0 + 3.0;

        double xp = random.NextDouble() * (s_width - xr - 2.0) + 1.0 + xr / 2.0;
        double yp = random.NextDouble() * (s_height - yr - 4.0) + 2.0 + yr / 2.0;
        double zp = random.NextDouble() * (s_width - zr - 2.0) + 1.0 + zr / 2.0;

        for (int xx = 1; xx < s_width - 1; xx++)
        {
            for (int zz = 1; zz < s_width - 1; zz++)
            {
                for (int yy = 1; yy < s_height - 1; yy++)
                {
                    double xd = (xx - xp) / (xr / 2.0);
                    double yd = (yy - yp) / (yr / 2.0);
                    double zd = (zz - zp) / (zr / 2.0);
                    if (xd * xd + yd * yd + zd * zd < 1.0)
                    {
                        plan->grid[Index(xx, yy, zz)] = true;
                    }
                }
            }
        }
    }
}

bool LakeFeature::Covers(const Plan &plan, int x, int z) const
{
    if (!plan.active || x < 0 || z < 0 || x >= s_width || z >= s_width)
    {
        return false;
    }

    for (int yy = 0; yy < s_height; yy++)
    {
        if (plan.grid[Index(x, yy, z)])
        {
            return true;
        }
    }

    return false;
}

bool LakeFeature::IsEdge(const Plan &plan, int x, int y, int z) const
{
    if (plan.grid[Index(x, y, z)])
    {
        return false;
    }

    return (x < s_width - 1 && plan.grid[Index(x + 1, y, z)]) || (x > 0 && plan.grid[Index(x - 1, y, z)]) ||
           (z < s_width - 1 && plan.grid[Index(x, y, z + 1)]) || (z > 0 && plan.grid[Index(x, y, z - 1)]) ||
           (y < s_height - 1 && plan.grid[Index(x, y + 1, z)]) || (y > 0 && plan.grid[Index(x, y - 1, z)]);
}

void LakeFeature::Place(Chunk &chunk, const Plan &plan, Block *liquid, Random &random) const
{
    if (!plan.active || plan.baseY < 0 || plan.baseY + s_height > Chunk::s_sizeY)
    {
        return;
    }

    for (int xx = 0; xx < s_width; xx++)
    {
        for (int zz = 0; zz < s_width; zz++)
        {
            for (int yy = 0; yy < s_height; yy++)
            {
                if (!IsEdge(plan, xx, yy, zz))
                {
                    continue;
                }

                Block *block = Block::ById(chunk.GetBlockId(xx, plan.baseY + yy, zz));
                if (yy >= s_height / 2 && block->IsLiquid())
                {
                    return;
                }

                if (yy < s_height / 2 && !block->IsSolid() && block != liquid)
                {
                    return;
                }
            }
        }
    }

    for (int xx = 0; xx < s_width; xx++)
    {
        for (int zz = 0; zz < s_width; zz++)
        {
            for (int yy = 0; yy < s_height; yy++)
            {
                if (plan.grid[Index(xx, yy, zz)])
                {
                    chunk.SetBlock(xx, plan.baseY + yy, zz, yy >= s_height / 2 ? m_air : liquid);
                }
            }
        }
    }

    for (int xx = 0; xx < s_width; xx++)
    {
        for (int zz = 0; zz < s_width; zz++)
        {
            for (int yy = s_height / 2; yy < s_height; yy++)
            {
                if (plan.grid[Index(xx, yy, zz)] && Block::ById(chunk.GetBlockId(xx, plan.baseY + yy - 1, zz)) == m_dirt)
                {
                    chunk.SetBlock(xx, plan.baseY + yy - 1, zz, m_grassBlock);
                }
            }
        }
    }

    if (!liquid->IsLava())
    {
        return;
    }

    for (int xx = 0; xx < s_width; xx++)
    {
        for (int zz = 0; zz < s_width; zz++)
        {
            for (int yy = 0; yy < s_height; yy++)
            {
                if (!IsEdge(plan, xx, yy, zz) || (yy >= s_height / 2 && random.NextInt(0, 1) == 0))
                {
                    continue;
                }

                if (Block::ById(chunk.GetBlockId(xx, plan.baseY + yy, zz))->IsSolid())
                {
                    chunk.SetBlock(xx, plan.baseY + yy, zz, m_stone);
                }
            }
        }
    }
}
