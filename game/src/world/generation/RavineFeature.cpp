#include "RavineFeature.h"

#include <cmath>
#include <numbers>

#include "../block/Block.h"
#include "NoiseCaveCarver.h"

RavineFeature::RavineFeature(uint32_t seed, int genDepth) : LargeFeature(seed)
{
    m_genDepth   = genDepth;
    m_air        = nullptr;
    m_lava       = nullptr;
    m_stone      = nullptr;
    m_dirt       = nullptr;
    m_grassBlock = nullptr;
    m_sand       = nullptr;
    m_waterStill = nullptr;
}

void RavineFeature::Apply(Chunk &chunk, const ChunkPos &chunkPos)
{
    m_air        = Block::ByName("air");
    m_lava       = Block::ByName("lava_still");
    m_stone      = Block::ByName("stone");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
    m_sand       = Block::ByName("sand");
    m_waterStill = Block::ByName("water_still");

    Random seedRandom(m_seed ^ 0x5a17e3u);
    uint64_t xScale = seedRandom.NextUInt64();
    uint64_t zScale = seedRandom.NextUInt64();

    for (int x = chunkPos.x - s_radius; x <= chunkPos.x + s_radius; x++)
    {
        for (int z = chunkPos.z - s_radius; z <= chunkPos.z + s_radius; z++)
        {
            uint64_t featureSeed = ((uint64_t) x * xScale) ^ ((uint64_t) z * zScale) ^ (uint64_t) m_seed;
            Random featureRandom(featureSeed);
            AddFeature(chunk, chunkPos, featureRandom, x, z);
        }
    }
}

void RavineFeature::AddFeature(Chunk &chunk, const ChunkPos &chunkPos, Random &random, int featureX, int featureZ)
{
    if (NextInt(random, 50) != 0)
    {
        return;
    }

    double xCave    = (double) (featureX * 16 + NextInt(random, 16));
    double yCave    = (double) (NextInt(random, NextInt(random, 40) + 8) + 20);
    double zCave    = (double) (featureZ * 16 + NextInt(random, 16));
    float yRot      = random.NextFloat() * std::numbers::pi_v<float> * 2.0f;
    float xRot      = ((random.NextFloat() - 0.5f) * 2.0f) / 8.0f;
    float thickness = (random.NextFloat() * 2.0f + random.NextFloat()) * 2.0f;
    AddTunnel(chunk, chunkPos, random.NextUInt64(), xCave, yCave, zCave, thickness, yRot, xRot, 3.0);
}

void RavineFeature::AddTunnel(Chunk &chunk, const ChunkPos &chunkPos, uint64_t seed, double xCave, double yCave, double zCave, float thickness,
                              float yRot, float xRot, double yScale)
{
    int xOffs   = chunkPos.x;
    int zOffs   = chunkPos.z;
    double xMid = (double) (xOffs * 16 + 8);
    double zMid = (double) (zOffs * 16 + 8);

    float yRota = 0.0f;
    float xRota = 0.0f;

    Random random(seed);

    int max  = s_radius * 16 - 16;
    int dist = max - NextInt(random, max / 4);

    float layerScale[Chunk::s_sizeY];
    float scale = 1.0f;
    for (int y = 0; y < m_genDepth; y++)
    {
        if (y == 0 || NextInt(random, 3) == 0)
        {
            scale = 1.0f + random.NextFloat() * random.NextFloat();
        }

        layerScale[y] = scale * scale;
    }

    for (int step = 0; step < dist; step++)
    {
        double rad  = 1.5 + sin((double) step * std::numbers::pi / (double) dist) * (double) thickness;
        double yRad = rad * yScale;
        rad *= (double) random.NextFloat() * 0.25 + 0.75;
        yRad *= (double) random.NextFloat() * 0.25 + 0.75;

        float xc = cosf(xRot);
        float xs = sinf(xRot);
        xCave += cosf(yRot) * xc;
        yCave += xs;
        zCave += sinf(yRot) * xc;

        xRot *= 0.7f;
        xRot += xRota * 0.05f;
        yRot += yRota * 0.05f;

        xRota *= 0.8f;
        yRota *= 0.5f;
        xRota += (random.NextFloat() - random.NextFloat()) * random.NextFloat() * 2.0f;
        yRota += (random.NextFloat() - random.NextFloat()) * random.NextFloat() * 4.0f;

        if (NextInt(random, 4) == 0)
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

        bool detectedWater = false;
        for (int xx = x0; !detectedWater && xx < x1; xx++)
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
                    if ((xdist * xdist + zdist * zdist) * (double) layerScale[yy] + ydist * ydist / 6.0 >= 1.0)
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

                    chunk.SetBlock(xx, yy, zz, yy < NoiseCaveCarver::s_lavaLevel ? m_lava : m_air);

                    if (yy >= 10 && hasGrass && Block::ById(chunk.GetBlockId(xx, yy - 1, zz)) == m_dirt)
                    {
                        Biome *biome = chunk.GetSurfaceBiome(xx, zz);
                        chunk.SetBlock(xx, yy - 1, zz, biome ? biome->GetTopBlock() : m_grassBlock);
                    }
                }
            }
        }
    }
}
