#include "NoiseCaveCarver.h"

#include "../block/Block.h"

NoiseCaveCarver::NoiseCaveCarver(uint32_t seed, bool caveDimension)
{
    m_caveDimension = caveDimension;
    m_maxY          = caveDimension ? Chunk::s_sizeY : 160;

    m_cheese.SetSeed((int) (seed ^ 0x3c1a5e));
    m_cheese.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_cheese.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_cheese.SetFractalOctaves(2);
    m_cheese.SetFrequency(1.0f);

    m_spaghettiA.SetSeed((int) (seed ^ 0x51a7e));
    m_spaghettiA.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_spaghettiA.SetFrequency(1.0f);

    m_spaghettiB.SetSeed((int) (seed ^ 0x7b90d));
    m_spaghettiB.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_spaghettiB.SetFrequency(1.0f);

    m_entrance.SetSeed((int) (seed ^ 0x2e47f1));
    m_entrance.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_entrance.SetFrequency(1.0f);

    // This matches the dripstone biome noise so openings land above dripstone caves
    m_dripstone.SetSeed((int) seed ^ 0x3d91);
    m_dripstone.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_dripstone.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_dripstone.SetFractalOctaves(2);
    m_dripstone.SetFrequency(1.0f);

    m_air        = Block::ByName("air");
    m_stone      = Block::ByName("stone");
    m_dirt       = Block::ByName("dirt");
    m_grassBlock = Block::ByName("grass_block");
    m_sand       = Block::ByName("sand");
    m_snowBlock  = Block::ByName("snow_block");
    m_packedIce  = Block::ByName("packed_ice");
    m_water      = Block::ByName("water_still");
}

int NoiseCaveCarver::GridIndex(int x, int y, int z)
{
    return (x * s_gridZ + z) * s_gridY + y;
}

double NoiseCaveCarver::Sample(const float *grid, int x, int y, int z)
{
    int gx    = x / s_cellWidth;
    int gy    = y / s_cellHeight;
    int gz    = z / s_cellWidth;
    double fx = (double) (x % s_cellWidth) / s_cellWidth;
    double fy = (double) (y % s_cellHeight) / s_cellHeight;
    double fz = (double) (z % s_cellWidth) / s_cellWidth;

    double c000 = grid[GridIndex(gx, gy, gz)];
    double c100 = grid[GridIndex(gx + 1, gy, gz)];
    double c010 = grid[GridIndex(gx, gy + 1, gz)];
    double c110 = grid[GridIndex(gx + 1, gy + 1, gz)];
    double c001 = grid[GridIndex(gx, gy, gz + 1)];
    double c101 = grid[GridIndex(gx + 1, gy, gz + 1)];
    double c011 = grid[GridIndex(gx, gy + 1, gz + 1)];
    double c111 = grid[GridIndex(gx + 1, gy + 1, gz + 1)];

    double c00 = c000 + (c100 - c000) * fx;
    double c10 = c010 + (c110 - c010) * fx;
    double c01 = c001 + (c101 - c001) * fx;
    double c11 = c011 + (c111 - c011) * fx;
    double c0  = c00 + (c10 - c00) * fy;
    double c1  = c01 + (c11 - c01) * fy;
    return c0 + (c1 - c0) * fz;
}

int NoiseCaveCarver::FindSurface(const Chunk &chunk, int x, int z, bool *underwater) const
{
    for (int y = m_maxY - 1; y > 0; y--)
    {
        Block *block = Block::ById(chunk.GetBlockId(x, y, z));
        if (block == m_air)
        {
            continue;
        }

        *underwater = block == m_water;
        while (y > 0 && Block::ById(chunk.GetBlockId(x, y, z)) == m_water)
        {
            y--;
        }

        return y;
    }

    *underwater = false;
    return 0;
}

// This is 0 outside dripstone caves and ramps to 1 inside them, sampled just under the cave biome top
double NoiseCaveCarver::GetDripstoneOpenness(int x, int surface, int z) const
{
    const float dripstoneScaleXZ = 1.0f / 128.0f;
    const float dripstoneScaleY  = 1.0f / 64.0f;
    const double fadeStart       = 0.22;
    const double fadeEnd         = 0.35;
    const int sampleDepth        = 16;

    int y        = surface - sampleDepth > 0 ? surface - sampleDepth : 0;
    double noise = m_dripstone.GetNoise((float) x * dripstoneScaleXZ, (float) y * dripstoneScaleY, (float) z * dripstoneScaleXZ);
    double t     = (noise - fadeStart) / (fadeEnd - fadeStart);
    if (t <= 0.0)
    {
        return 0.0;
    }

    if (t >= 1.0)
    {
        return 1.0;
    }

    return t * t * (3.0 - 2.0 * t);
}

bool NoiseCaveCarver::ShouldCarve(const float *cheese, const float *spaghettiA, const float *spaghettiB, int x, int y, int z, double entrance,
                                  int surface, bool underwater, double openness) const
{
    const double cheeseThreshold      = 0.45;
    const int cheeseSurfaceBuffer     = 14;
    const double cheeseSurfaceFalloff = 0.06 - 0.04 * openness;
    const int floorFadeHeight         = 6;
    const double floorFalloff         = 0.1;
    const double spaghettiWidth       = 0.0045;
    const int underwaterBuffer        = 10;
    const int entranceDepth           = 12;
    const double entranceThreshold    = 0.5 - 0.4 * openness;

    int depth = surface - y;
    if (underwater && depth < underwaterBuffer)
    {
        return false;
    }

    double floorFade = y < s_minCaveY + floorFadeHeight ? (double) (s_minCaveY + floorFadeHeight - y) : 0.0;

    double threshold = cheeseThreshold + floorFade * floorFalloff;
    if (depth < cheeseSurfaceBuffer)
    {
        threshold += (double) (cheeseSurfaceBuffer - depth) * cheeseSurfaceFalloff;
    }

    if (Sample(cheese, x, y, z) > threshold)
    {
        return true;
    }

    double width = spaghettiWidth * (1.0 - floorFade / floorFadeHeight);
    if (depth < entranceDepth)
    {
        if (entrance < entranceThreshold)
        {
            return false;
        }

        width *= (2.0 - (double) depth / entranceDepth) * (1.0 + openness);
    }

    double a = Sample(spaghettiA, x, y, z);
    double b = Sample(spaghettiB, x, y, z);
    return a * a + b * b < width;
}

void NoiseCaveCarver::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const float cheeseScaleXZ    = 1.0f / 80.0f;
    const float cheeseScaleY     = 1.0f / 40.0f;
    const float spaghettiScaleXZ = 1.0f / 56.0f;
    const float spaghettiScaleY  = 1.0f / 40.0f;
    const float entranceScale    = 1.0f / 48.0f;
    const int minEntranceDepth   = 4;

    float cheese[s_gridX * s_gridY * s_gridZ];
    float spaghettiA[s_gridX * s_gridY * s_gridZ];
    float spaghettiB[s_gridX * s_gridY * s_gridZ];
    for (int gx = 0; gx < s_gridX; gx++)
    {
        for (int gz = 0; gz < s_gridZ; gz++)
        {
            float x = (float) (pos.x * Chunk::s_sizeX + gx * s_cellWidth);
            float z = (float) (pos.z * Chunk::s_sizeZ + gz * s_cellWidth);
            for (int gy = 0; gy <= m_maxY / s_cellHeight; gy++)
            {
                float y           = (float) (gy * s_cellHeight);
                int index         = GridIndex(gx, gy, gz);
                cheese[index]     = m_cheese.GetNoise(x * cheeseScaleXZ, y * cheeseScaleY, z * cheeseScaleXZ);
                spaghettiA[index] = m_spaghettiA.GetNoise(x * spaghettiScaleXZ, y * spaghettiScaleY, z * spaghettiScaleXZ);
                spaghettiB[index] = m_spaghettiB.GetNoise(x * spaghettiScaleXZ, y * spaghettiScaleY, z * spaghettiScaleXZ);
            }
        }
    }

    bool carve[Chunk::s_sizeY];
    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            bool underwater = false;
            // The cave dimension has no surface, so we put it out of reach of the surface falloff and entrance rules
            int surface     = m_caveDimension ? m_maxY + Chunk::s_sizeY : FindSurface(chunk, x, z, &underwater);
            int top         = surface < m_maxY - 1 ? surface : m_maxY - 1;
            double openness = GetDripstoneOpenness(pos.x * Chunk::s_sizeX + x, surface, pos.z * Chunk::s_sizeZ + z);
            double entrance =
                    m_entrance.GetNoise((float) (pos.x * Chunk::s_sizeX + x) * entranceScale, (float) (pos.z * Chunk::s_sizeZ + z) * entranceScale);
            for (int y = s_minCaveY; y <= top; y++)
            {
                Block *block = Block::ById(chunk.GetBlockId(x, y, z));
                carve[y]     = (block == m_stone || block == m_dirt || block == m_grassBlock || block == m_sand || block == m_snowBlock ||
                                block == m_packedIce) &&
                               ShouldCarve(cheese, spaghettiA, spaghettiB, x, y, z, entrance, surface, underwater, openness);
            }

            if (top >= s_minCaveY && carve[top])
            {
                int bottom = top;
                while (bottom > s_minCaveY && carve[bottom - 1])
                {
                    bottom--;
                }

                if (top - bottom + 1 < minEntranceDepth)
                {
                    for (int y = bottom; y <= top; y++)
                    {
                        carve[y] = false;
                    }
                }
            }

            for (int y = s_minCaveY; y <= top; y++)
            {
                if (carve[y])
                {
                    chunk.SetBlock(x, y, z, m_air);
                }
            }
        }
    }
}
