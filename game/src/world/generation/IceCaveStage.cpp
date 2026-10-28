#include "IceCaveStage.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <string>

#include "../../utils/Direction.h"
#include "../block/Block.h"
#include "../block/IceSheetBlock.h"
#include "../block/PointedDripstoneBlock.h"
#include "DripstoneStage.h"
#include "TerrainGenerator.h"

IceCaveStage::IceCaveStage(TerrainGenerator *generator)
{
    m_generator = generator;
    m_iceCaves  = Biome::ByName("ice_caves");
    m_glacier   = Biome::ByName("glacier");
    m_iceZoneNoise.SetSeed((int) generator->GetSeed() ^ 0x2ea1);
    m_iceZoneNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_iceZoneNoise.SetFrequency(1.0f);
}

void IceCaveStage::PlaceIceCrystal(Chunk &chunk, Random &random, int x, int y, int z) const
{
    Block *ice          = Block::ByName("ice");
    Block *enchantedIce = Block::ByName("enchanted_ice");
    int halfHeight      = random.NextInt(3, 5);
    int radius          = random.NextInt(2, 3);
    int centerY         = y + halfHeight / 2;

    for (int dy = -halfHeight; dy <= halfHeight; dy++)
    {
        int blockY = centerY + dy;
        if (blockY <= 1 || blockY >= Chunk::s_sizeY - 1)
        {
            continue;
        }

        int layerRadius = (radius * (halfHeight - std::abs(dy)) + halfHeight / 2) / halfHeight;
        for (int dx = -layerRadius; dx <= layerRadius; dx++)
        {
            for (int dz = -layerRadius; dz <= layerRadius; dz++)
            {
                if (std::abs(dx) + std::abs(dz) > layerRadius)
                {
                    continue;
                }

                int blockX = x + dx;
                int blockZ = z + dz;
                if (blockX < 0 || blockX >= Chunk::s_sizeX || blockZ < 0 || blockZ >= Chunk::s_sizeZ)
                {
                    continue;
                }

                Block *current = Block::ById(chunk.GetBlockId(blockX, blockY, blockZ));
                if (current && current->IsLiquid())
                {
                    continue;
                }

                chunk.SetBlock(blockX, blockY, blockZ, ice);
                chunk.SetBlockData(blockX, blockY, blockZ, 0);
            }
        }
    }

    chunk.SetBlock(x, centerY, z, enchantedIce);
    chunk.SetBlockData(x, centerY, z, 0);
    if (random.NextInt(0, 1) == 0)
    {
        int offsetY = random.NextInt(0, 1) == 0 ? 1 : -1;
        chunk.SetBlock(x, centerY + offsetY, z, enchantedIce);
        chunk.SetBlockData(x, centerY + offsetY, z, 0);
    }
}

void IceCaveStage::PlaceBrittleIcePit(Chunk &chunk, Random &random, int x, int y, int z) const
{
    const int maxRadius = 3;
    const int maxDepth  = 12;

    Block *packedIce  = Block::ByName("packed_ice");
    Block *brittleIce = Block::ByName("brittle_ice");
    Block *liquid     = Block::ByName(random.NextInt(0, 4) == 0 ? "lava_still" : "water_still");
    int radius        = random.NextInt(1, 2);
    int depth         = std::min(random.NextInt(0, 3) == 0 ? random.NextInt(6, maxDepth) : random.NextInt(3, 5), y - 1);
    int liquidDepth   = random.NextInt(1, 2);

    // Layer 0 is the brittle ice and the layer under it copies its footprint so the hole never shows past the ice
    bool carved[maxDepth + 1][maxRadius * 2 + 1][maxRadius * 2 + 1] = {};
    for (int dy = 0; dy <= depth; dy++)
    {
        int layerRadius = radius + (dy > 1 && dy < depth ? 1 : 0);
        for (int dx = -maxRadius; dx <= maxRadius; dx++)
        {
            for (int dz = -maxRadius; dz <= maxRadius; dz++)
            {
                float limit                                = layerRadius + 0.5f - random.NextFloat() * 0.6f;
                carved[dy][dx + maxRadius][dz + maxRadius] = dy == 1 ? carved[0][dx + maxRadius][dz + maxRadius] : dx * dx + dz * dz <= limit * limit;
                if (dy == 0 && carved[0][dx + maxRadius][dz + maxRadius])
                {
                    Block *floor = Block::ById(chunk.GetBlockId(x + dx, y, z + dz));
                    carved[0][dx + maxRadius][dz + maxRadius] =
                            (floor == packedIce || TerrainGenerator::IsStone(floor)) && chunk.GetBlockId(x + dx, y + 1, z + dz) == 0;
                }
            }
        }
    }

    if (!carved[0][maxRadius][maxRadius])
    {
        return;
    }

    for (int dy = 0; dy <= depth; dy++)
    {
        for (int dx = -maxRadius; dx <= maxRadius; dx++)
        {
            for (int dz = -maxRadius; dz <= maxRadius; dz++)
            {
                if (!carved[dy][dx + maxRadius][dz + maxRadius])
                {
                    continue;
                }

                Block *block = Block::ById(chunk.GetBlockId(x + dx, y - dy, z + dz));
                if (block != packedIce && !TerrainGenerator::IsStone(block))
                {
                    return;
                }

                if (dy == 0)
                {
                    continue;
                }

                // The pit has to be sealed so its liquid can't leak into a cave next to it
                for (int i = 0; i < 6; i++)
                {
                    Direction *side = Direction::ById(i + 1);
                    int ny          = dy - side->dy;
                    int nx          = dx + side->dx;
                    int nz          = dz + side->dz;
                    if (ny < 0)
                    {
                        continue;
                    }

                    if (ny <= depth && std::abs(nx) <= maxRadius && std::abs(nz) <= maxRadius && carved[ny][nx + maxRadius][nz + maxRadius])
                    {
                        continue;
                    }

                    uint32_t neighbor = chunk.GetBlockId(x + nx, y - ny, z + nz);
                    if (neighbor == 0 || Block::ById(neighbor)->IsLiquid())
                    {
                        return;
                    }
                }
            }
        }
    }

    for (int dy = 0; dy <= depth; dy++)
    {
        Block *fill = dy == 0 ? brittleIce : dy > depth - liquidDepth ? liquid : Block::ByName("air");
        for (int dx = -maxRadius; dx <= maxRadius; dx++)
        {
            for (int dz = -maxRadius; dz <= maxRadius; dz++)
            {
                if (carved[dy][dx + maxRadius][dz + maxRadius])
                {
                    chunk.SetBlock(x + dx, y - dy, z + dz, fill);
                    chunk.SetBlockData(x + dx, y - dy, z + dz, 0);
                }
            }
        }
    }
}

void IceCaveStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const float zoneScale            = 1.0f / 24.0f;
    const int maxIcicleLength        = 3;
    const int maxGlacierIcicleLength = 6;
    const int crystalMargin          = 3;
    const int pitMargin              = 4;

    Block *packedIce              = Block::ByName("packed_ice");
    Block *snow                   = Block::ByName("snow");
    IceSheetBlock *iceSheet       = (IceSheetBlock *) Block::ByName("ice_sheet");
    PointedDripstoneBlock *icicle = (PointedDripstoneBlock *) Block::ByName("icicle");
    int baseX                     = pos.x * Chunk::s_sizeX;
    int baseZ                     = pos.z * Chunk::s_sizeZ;
    const char *oreNames[]        = {"coal", "iron", "gold", "diamond", "emerald", "lapis", "redstone"};
    Block *stoneOres[std::size(oreNames)];
    Block *iceOres[std::size(oreNames)];
    for (size_t i = 0; i < std::size(oreNames); i++)
    {
        stoneOres[i] = Block::ByName(std::string(oreNames[i]) + "_ore");
        iceOres[i]   = Block::ByName(std::string("ice_") + oreNames[i] + "_ore");
    }

    Random random(m_generator->GetChunkSeed(pos, 0x6a09e667f3bcc909ULL));

    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            bool glacier    = chunk.GetSurfaceBiome(x, z) == m_glacier;
            int icicleLimit = glacier ? maxGlacierIcicleLength : maxIcicleLength;
            for (int y = 1; y < Chunk::s_sizeY - 1; y++)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_iceCaves)
                {
                    continue;
                }

                Block *block = Block::ById(chunk.GetBlockId(x, y, z));
                Block **ore  = std::find(stoneOres, stoneOres + std::size(oreNames), block);
                bool isOre   = ore != stoneOres + std::size(oreNames);
                if (!isOre && !TerrainGenerator::IsStone(block))
                {
                    continue;
                }

                if (!glacier && m_iceZoneNoise.GetNoise((baseX + x) * zoneScale, y * zoneScale, (baseZ + z) * zoneScale) <= 0.1f)
                {
                    continue;
                }

                if (isOre)
                {
                    chunk.SetBlock(x, y, z, iceOres[ore - stoneOres]);
                    continue;
                }

                bool exposed = chunk.GetBlockId(x, y + 1, z) == 0 || chunk.GetBlockId(x, y - 1, z) == 0 ||
                               (x > 0 && chunk.GetBlockId(x - 1, y, z) == 0) || (x < Chunk::s_sizeX - 1 && chunk.GetBlockId(x + 1, y, z) == 0) ||
                               (z > 0 && chunk.GetBlockId(x, y, z - 1) == 0) || (z < Chunk::s_sizeZ - 1 && chunk.GetBlockId(x, y, z + 1) == 0);
                if (exposed || glacier || random.NextInt(0, 1) == 0)
                {
                    chunk.SetBlock(x, y, z, packedIce);
                }
            }

            for (int y = Chunk::s_sizeY - 2; y > 1; y--)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_iceCaves || chunk.GetBlockId(x, y, z) != 0)
                {
                    continue;
                }

                Block *above = Block::ById(chunk.GetBlockId(x, y + 1, z));
                if ((above == packedIce || TerrainGenerator::IsStone(above)) && random.NextInt(0, glacier ? 4 : 9) == 0)
                {
                    int space = 0;
                    while (space < icicleLimit + 1 && y - space > 0 && chunk.GetBlockId(x, y - space, z) == 0)
                    {
                        space++;
                    }

                    int length = std::min(random.NextInt(1, icicleLimit), space - 1);
                    if (length > 0)
                    {
                        DripstoneStage::PlaceColumn(chunk, icicle, x, y, z, Direction::down, length);
                    }
                }
            }
        }
    }

    for (int attempt = 0; attempt < 6; attempt++)
    {
        if (random.NextInt(0, 2) != 0)
        {
            continue;
        }

        int x = random.NextInt(crystalMargin, Chunk::s_sizeX - 1 - crystalMargin);
        int z = random.NextInt(crystalMargin, Chunk::s_sizeZ - 1 - crystalMargin);
        int y = random.NextInt(2, Chunk::s_sizeY - 8);
        while (y < Chunk::s_sizeY - 8 &&
               !(chunk.GetBlockId(x, y, z) == 0 && chunk.GetBiomeAt(x, y, z) == m_iceCaves && chunk.GetBlockId(x, y - 1, z) != 0))
        {
            y++;
        }

        if (y < Chunk::s_sizeY - 8)
        {
            PlaceIceCrystal(chunk, random, x, y, z);
        }
    }

    for (int attempt = 0; attempt < 4; attempt++)
    {
        int x = random.NextInt(pitMargin, Chunk::s_sizeX - 1 - pitMargin);
        int z = random.NextInt(pitMargin, Chunk::s_sizeZ - 1 - pitMargin);
        int y = random.NextInt(8, Chunk::s_sizeY - 8);
        while (y < Chunk::s_sizeY - 8 &&
               !(chunk.GetBlockId(x, y + 1, z) == 0 && chunk.GetBiomeAt(x, y + 1, z) == m_iceCaves && chunk.GetBlockId(x, y, z) != 0))
        {
            y++;
        }

        if (y < Chunk::s_sizeY - 8)
        {
            PlaceBrittleIcePit(chunk, random, x, y, z);
        }
    }

    // Snow and sheets go last so they also cover the stone around crystals and icicle roots
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = 1; y < Chunk::s_sizeY - 1; y++)
            {
                if (chunk.GetBiomeAt(x, y, z) != m_iceCaves || chunk.GetBlockId(x, y, z) != 0)
                {
                    continue;
                }

                Block *below = Block::ById(chunk.GetBlockId(x, y - 1, z));
                if ((below == packedIce || TerrainGenerator::IsStone(below)) && random.NextInt(0, 2) == 0)
                {
                    chunk.SetBlock(x, y, z, snow);
                    chunk.SetBlockData(x, y, z, 0);
                    continue;
                }

                uint16_t data = 0;
                for (int i = 0; i < 6; i++)
                {
                    Direction *side = Direction::ById(i + 1);
                    int nx          = x + side->dx;
                    int nz          = z + side->dz;
                    if (nx < 0 || nx >= Chunk::s_sizeX || nz < 0 || nz >= Chunk::s_sizeZ)
                    {
                        continue;
                    }

                    if (TerrainGenerator::IsStone(Block::ById(chunk.GetBlockId(nx, y + side->dy, nz))) && random.NextInt(0, 2) != 0)
                    {
                        data = iceSheet->WithFace(data, side);
                    }
                }

                if (data != 0)
                {
                    chunk.SetBlock(x, y, z, iceSheet);
                    chunk.SetBlockData(x, y, z, data);
                }
            }
        }
    }
}
