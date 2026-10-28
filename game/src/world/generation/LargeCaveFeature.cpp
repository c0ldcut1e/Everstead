#include "LargeCaveFeature.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../block/Block.h"
#include "../block/BlockRegistry.h"
#include "NoiseCaveCarver.h"

LargeCaveFeature::LargeCaveFeature(uint32_t seed, int genDepth) : LargeFeature(seed)
{
    m_genDepth   = genDepth;
    m_air        = nullptr;
    m_lava       = nullptr;
    m_stone      = nullptr;
    m_dirt       = nullptr;
    m_grassBlock = nullptr;
    m_sand       = nullptr;
    m_waterStill = nullptr;
    m_waterFlow  = nullptr;
}

void LargeCaveFeature::Apply(Chunk &chunk, const ChunkPos &chunkPos)
{
    m_air        = Block::ByName("air");
    m_lava       = Block::ByName("lava_still");
    m_stone      = Block::ByName("stone");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
    m_sand       = Block::ByName("sand");
    m_waterStill = Block::ByName("water_still");
    m_waterFlow  = Block::ByName("water_flow");

    Random seedRandom(m_seed);
    uint64_t xScale = seedRandom.NextUInt64();
    uint64_t zScale = seedRandom.NextUInt64();

    bool flooded = false;
    for (int x = chunkPos.x - s_radius; x <= chunkPos.x + s_radius; x++)
    {
        for (int z = chunkPos.z - s_radius; z <= chunkPos.z + s_radius; z++)
        {
            uint64_t featureSeed = ((uint64_t) x * xScale) ^ ((uint64_t) z * zScale) ^ (uint64_t) m_seed;
            Random featureRandom(featureSeed);
            flooded = AddFeature(chunk, chunkPos, featureRandom, x, z) || flooded;
        }
    }

    if (!flooded)
    {
        return;
    }

    // Still water never ticks, so water touching air or a chunk edge starts flowing to drain into caves it cut open
    uint32_t airId             = BlockRegistry::GetInstance()->IdOf(m_air);
    uint32_t waterId           = BlockRegistry::GetInstance()->IdOf(m_waterStill);
    constexpr int kFloodedMaxY = 80;
    for (int y = std::max(NoiseCaveCarver::s_minCaveY, 1); y < kFloodedMaxY; y++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                if (chunk.GetBlockId(x, y, z) != waterId)
                {
                    continue;
                }

                bool edge = x == 0 || z == 0 || x == Chunk::s_sizeX - 1 || z == Chunk::s_sizeZ - 1;
                if (edge || chunk.GetBlockId(x, y - 1, z) == airId || chunk.GetBlockId(x - 1, y, z) == airId ||
                    chunk.GetBlockId(x + 1, y, z) == airId || chunk.GetBlockId(x, y, z - 1) == airId || chunk.GetBlockId(x, y, z + 1) == airId)
                {
                    chunk.SetBlock(x, y, z, m_waterFlow);
                }
            }
        }
    }
}

bool LargeCaveFeature::AddFeature(Chunk &chunk, const ChunkPos &chunkPos, Random &random, int featureX, int featureZ)
{
    int caves = NextInt(random, NextInt(random, NextInt(random, 40) + 1) + 1);
    if (NextInt(random, 15) != 0)
    {
        caves = 0;
    }

    for (int cave = 0; cave < caves; cave++)
    {
        double xCave = (double) (featureX * 16 + NextInt(random, 16));
        double yCave = (double) NextInt(random, NextInt(random, m_genDepth - 8) + 8);
        double zCave = (double) (featureZ * 16 + NextInt(random, 16));

        int tunnels = 1;
        if (NextInt(random, 4) == 0)
        {
            AddRoom(chunk, chunkPos, random, random.NextUInt64(), xCave, yCave, zCave);
            tunnels += NextInt(random, 4);
        }

        for (int tunnel = 0; tunnel < tunnels; tunnel++)
        {
            float yRot      = random.NextFloat() * std::numbers::pi_v<float> * 2.0f;
            float xRot      = ((random.NextFloat() - 0.5f) * 2.0f) / 8.0f;
            float thickness = random.NextFloat() * 2.0f + random.NextFloat();
            if (NextInt(random, 10) == 0)
            {
                thickness *= random.NextFloat() * random.NextFloat() * 3.0f + 1.0f;
            }

            AddTunnel(chunk, chunkPos, random.NextUInt64(), xCave, yCave, zCave, thickness, yRot, xRot, 0, 0, 1.0, false);
        }
    }

    if (NextInt(random, 400) != 0)
    {
        return false;
    }

    double xCave = (double) (featureX * 16 + NextInt(random, 16));
    double yCave = (double) (16 + NextInt(random, 32));
    double zCave = (double) (featureZ * 16 + NextInt(random, 16));
    AddTunnel(chunk, chunkPos, random.NextUInt64(), xCave, yCave, zCave, 10.0f + random.NextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5, true);

    int tunnels = 3 + NextInt(random, 4);
    for (int tunnel = 0; tunnel < tunnels; tunnel++)
    {
        float yRot      = random.NextFloat() * std::numbers::pi_v<float> * 2.0f;
        float xRot      = ((random.NextFloat() - 0.5f) * 2.0f) / 8.0f;
        float thickness = 3.0f + random.NextFloat() * 3.0f;
        AddTunnel(chunk, chunkPos, random.NextUInt64(), xCave, yCave, zCave, thickness, yRot, xRot, 0, 0, 1.0, true);
    }

    return true;
}

void LargeCaveFeature::AddRoom(Chunk &chunk, const ChunkPos &chunkPos, Random &featureRandom, uint64_t seed, double xRoom, double yRoom, double zRoom)
{
    AddTunnel(chunk, chunkPos, seed, xRoom, yRoom, zRoom, 1.0f + featureRandom.NextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5, false);
}

void LargeCaveFeature::AddTunnel(Chunk &chunk, const ChunkPos &chunkPos, uint64_t seed, double xCave, double yCave, double zCave, float thickness,
                                 float yRot, float xRot, int step, int dist, double yScale, bool flooded)
{
    int xOffs   = chunkPos.x;
    int zOffs   = chunkPos.z;
    double xMid = (double) (xOffs * 16 + 8);
    double zMid = (double) (zOffs * 16 + 8);

    float yRota = 0.0f;
    float xRota = 0.0f;

    Random random(seed);

    if (dist <= 0)
    {
        int max = s_radius * 16 - 16;
        dist    = max - NextInt(random, max / 4);
    }

    bool singleStep = false;
    if (step == -1)
    {
        step       = dist / 2;
        singleStep = true;
    }

    int splitPoint = NextInt(random, dist / 2) + dist / 4;
    bool steep     = NextInt(random, 10) == 0;

    for (; step < dist; step++)
    {
        double rad  = 2.2 + sin((double) step * std::numbers::pi / (double) dist) * (double) thickness;
        double yRad = rad * yScale;

        float xc = cosf(xRot);
        float xs = sinf(xRot);
        xCave += cosf(yRot) * xc;
        yCave += xs;
        zCave += sinf(yRot) * xc;

        xRot *= steep ? 0.85f : 0.7f;
        xRot += xRota * 0.1f;
        yRot += yRota * 0.1f;

        xRota *= 0.9f;
        yRota *= 0.75f;
        xRota += (random.NextFloat() - random.NextFloat()) * random.NextFloat() * 2.0f;
        yRota += (random.NextFloat() - random.NextFloat()) * random.NextFloat() * 4.0f;

        if (!singleStep && step == splitPoint && thickness > 1.0f && dist > 0)
        {
            AddTunnel(chunk, chunkPos, random.NextUInt64(), xCave, yCave, zCave, random.NextFloat() + 1.0f, yRot - std::numbers::pi_v<float> / 2.0f,
                      xRot / 3.0f, step, dist, 1.0, flooded);
            AddTunnel(chunk, chunkPos, random.NextUInt64(), xCave, yCave, zCave, random.NextFloat() + 1.0f, yRot + std::numbers::pi_v<float> / 2.0f,
                      xRot / 3.0f, step, dist, 1.0, flooded);
            return;
        }

        if (!singleStep && NextInt(random, 4) == 0)
        {
            continue;
        }

        double xd        = xCave - xMid;
        double zd        = zCave - zMid;
        double remaining = (double) (dist - step);
        double rr        = ((double) thickness + 2.0) + 16.0;
        if (xd * xd + zd * zd - remaining * remaining > rr * rr)
        {
            return;
        }

        if (xCave < xMid - 16.0 - rad * 2.0 || zCave < zMid - 16.0 - rad * 2.0 || xCave > xMid + 16.0 + rad * 2.0 || zCave > zMid + 16.0 + rad * 2.0)
        {
            continue;
        }

        int x0 = (int) floor(xCave - rad) - xOffs * 16 - 1;
        int x1 = (int) floor(xCave + rad) - xOffs * 16 + 1;
        int y0 = (int) floor(yCave - yRad) - 1;
        int y1 = (int) floor(yCave + yRad) + 1;
        int z0 = (int) floor(zCave - rad) - zOffs * 16 - 1;
        int z1 = (int) floor(zCave + rad) - zOffs * 16 + 1;

        if (x0 < 0)
        {
            x0 = 0;
        }
        if (x1 > Chunk::s_sizeX)
        {
            x1 = Chunk::s_sizeX;
        }
        if (y0 < 1)
        {
            y0 = 1;
        }
        if (y1 > m_genDepth - 8)
        {
            y1 = m_genDepth - 8;
        }
        if (z0 < 0)
        {
            z0 = 0;
        }
        if (z1 > Chunk::s_sizeZ)
        {
            z1 = Chunk::s_sizeZ;
        }

        // A flooded cave would see its own water and stop carving, so only dry caves look for water
        bool detectedWater = false;
        for (int xx = x0; !flooded && !detectedWater && xx < x1; xx++)
        {
            for (int zz = z0; !detectedWater && zz < z1; zz++)
            {
                for (int yy = y1 + 1; !detectedWater && yy >= y0 - 1; yy--)
                {
                    if (yy < 0 || yy >= m_genDepth)
                    {
                        continue;
                    }

                    if (Block::ById(chunk.GetBlockId(xx, yy, zz)) == m_waterStill)
                    {
                        detectedWater = true;
                    }

                    if (yy != y0 - 1 && xx != x0 && xx != x1 - 1 && zz != z0 && zz != z1 - 1)
                    {
                        yy = y0;
                    }
                }
            }
        }

        if (detectedWater)
        {
            continue;
        }

        for (int xx = x0; xx < x1; xx++)
        {
            double xdist = ((double) (xx + xOffs * 16) + 0.5 - xCave) / rad;
            for (int zz = z0; zz < z1; zz++)
            {
                double zdist  = ((double) (zz + zOffs * 16) + 0.5 - zCave) / rad;
                bool hasGrass = false;
                if (xdist * xdist + zdist * zdist >= 1.0)
                {
                    continue;
                }

                for (int yy = y1 - 1; yy >= y0; yy--)
                {
                    double ydist = ((double) yy + 0.5 - yCave) / yRad;
                    if (ydist <= -0.7 || xdist * xdist + ydist * ydist + zdist * zdist >= 1.0)
                    {
                        continue;
                    }

                    if (yy < NoiseCaveCarver::s_minCaveY)
                    {
                        continue;
                    }

                    Block *block = Block::ById(chunk.GetBlockId(xx, yy, zz));
                    if (block == m_grassBlock)
                    {
                        hasGrass = true;
                    }

                    if (block != m_stone && block != m_dirt && block != m_grassBlock && block != m_sand)
                    {
                        continue;
                    }

                    chunk.SetBlock(xx, yy, zz, flooded ? m_waterStill : (yy < NoiseCaveCarver::s_lavaLevel ? m_lava : m_air));

                    if (!flooded && yy >= 10 && hasGrass && Block::ById(chunk.GetBlockId(xx, yy - 1, zz)) == m_dirt)
                    {
                        Biome *biome = chunk.GetSurfaceBiome(xx, zz);
                        chunk.SetBlock(xx, yy - 1, zz, biome ? biome->GetTopBlock() : m_grassBlock);
                    }
                }
            }
        }

        if (singleStep)
        {
            break;
        }
    }
}
