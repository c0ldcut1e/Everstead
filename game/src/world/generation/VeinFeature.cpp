#include "VeinFeature.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../block/Block.h"
#include "TerrainGenerator.h"

void VeinFeature::Place(Chunk &chunk, const ChunkPos &pos, Random &random, int x, int y, int z, Block *block, Block *target, int size)
{
    int baseX = pos.x * Chunk::s_sizeX;
    int baseZ = pos.z * Chunk::s_sizeZ;

    float angle  = random.NextFloat() * (float) std::numbers::pi;
    float spread = (float) size / 8.0f;
    double x0    = x + sin(angle) * spread;
    double x1    = x - sin(angle) * spread;
    double z0    = z + cos(angle) * spread;
    double z1    = z - cos(angle) * spread;
    double y0    = y + random.NextInt(0, 2) - 2;
    double y1    = y + random.NextInt(0, 2) - 2;

    for (int i = 0; i < size; i++)
    {
        float t       = (float) i / (float) size;
        double cx     = x0 + (x1 - x0) * t;
        double cy     = y0 + (y1 - y0) * t;
        double cz     = z0 + (z1 - z0) * t;
        double scale  = random.NextDouble() * size / 16.0;
        double radius = ((sin(std::numbers::pi * t) + 1.0) * scale + 1.0) / 2.0;
        int minX      = std::max((int) floor(cx - radius), baseX);
        int maxX      = std::min((int) floor(cx + radius), baseX + Chunk::s_sizeX - 1);
        int minY      = std::max((int) floor(cy - radius), 1);
        int maxY      = std::min((int) floor(cy + radius), TerrainGenerator::s_genDepth - 1);
        int minZ      = std::max((int) floor(cz - radius), baseZ);
        int maxZ      = std::min((int) floor(cz + radius), baseZ + Chunk::s_sizeZ - 1);
        for (int bx = minX; bx <= maxX; bx++)
        {
            double dx = (bx + 0.5 - cx) / radius;
            for (int by = minY; by <= maxY; by++)
            {
                double dy = (by + 0.5 - cy) / radius;
                for (int bz = minZ; bz <= maxZ; bz++)
                {
                    double dz = (bz + 0.5 - cz) / radius;
                    if (dx * dx + dy * dy + dz * dz < 1.0 && Block::ById(chunk.GetBlockId(bx - baseX, by, bz - baseZ)) == target)
                    {
                        chunk.SetBlock(bx - baseX, by, bz - baseZ, block);
                    }
                }
            }
        }
    }
}
