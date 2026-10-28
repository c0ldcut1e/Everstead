#include "TerrainGenerator.h"

#include <algorithm>
#include <cmath>

#include "../block/Block.h"
#include "BadlandsStage.h"
#include "CaveBiomeStage.h"
#include "CaveDimensionBiomeStage.h"
#include "ClayStage.h"
#include "DesertCaveStage.h"
#include "DesertPlantStage.h"
#include "DripstoneStage.h"
#include "DungeonStage.h"
#include "ForestRockStage.h"
#include "GlacierStage.h"
#include "HotDepthStage.h"
#include "IceCaveStage.h"
#include "IceSpikeStage.h"
#include "JungleStage.h"
#include "LakeStage.h"
#include "LargeCaveFeature.h"
#include "LavaHardenStage.h"
#include "LilyPadStage.h"
#include "MagmaDepthsStage.h"
#include "NoiseCaveCarver.h"
#include "OreStage.h"
#include "OvergrownHollowsStage.h"
#include "RavineFeature.h"
#include "SandLayerStage.h"
#include "SnowStage.h"
#include "SpringStage.h"
#include "StoneVariantStage.h"
#include "SugarCaneStage.h"
#include "TreeStage.h"
#include "VegetationStage.h"

TerrainGenerator::TerrainGenerator(uint32_t seed, const WorldGenSettings &settings)
    : m_noiseRandom(seed), m_biomeSource(seed, settings), m_lowerNoise(m_noiseRandom, 16), m_upperNoise(m_noiseRandom, 16),
      m_selectorNoise(m_noiseRandom, 8), m_surfaceNoise(m_noiseRandom, 4), m_depthNoise(m_noiseRandom, 16)
{
    m_seed       = seed;
    m_settings   = settings;
    m_flatHeight = 0;
    m_flatVoid   = true;
    for (size_t i = 0; i < settings.flatLayers.size(); i++)
    {
        m_flatHeight += settings.flatLayers[i].height;
        m_flatVoid = m_flatVoid && settings.flatLayers[i].block == "air";
    }

    m_flatHeight = m_flatVoid ? 65 : std::min(m_flatHeight, Chunk::s_sizeY);

    for (int xb = -2; xb <= 2; xb++)
    {
        for (int zb = -2; zb <= 2; zb++)
        {
            m_blendWeights[(xb + 2) + (zb + 2) * 5] = 10.0 / sqrt((double) (xb * xb + zb * zb) + 0.2);
        }
    }

    if (settings.worldType == eWorldType_Caves)
    {
        m_stages.push_back(std::make_unique<NoiseCaveCarver>(seed, true));
        m_stages.push_back(std::make_unique<LargeCaveFeature>(seed, Chunk::s_sizeY));
        m_stages.push_back(std::make_unique<RavineFeature>(seed, Chunk::s_sizeY));
        m_stages.push_back(std::make_unique<CaveDimensionBiomeStage>(seed));
        m_stages.push_back(std::make_unique<StoneVariantStage>(this));
        m_stages.push_back(std::make_unique<OreStage>(this));
        m_stages.push_back(std::make_unique<DripstoneStage>(this));
        m_stages.push_back(std::make_unique<IceCaveStage>(this));
        m_stages.push_back(std::make_unique<DesertCaveStage>(this));
        m_stages.push_back(std::make_unique<OvergrownHollowsStage>(this));
        m_stages.push_back(std::make_unique<MagmaDepthsStage>(this));
        m_stages.push_back(std::make_unique<SpringStage>(this, Block::ByName("water_flow"), false));
        m_stages.push_back(std::make_unique<SpringStage>(this, Block::ByName("lava_flow"), true));
        m_stages.push_back(std::make_unique<LavaHardenStage>());
        return;
    }

    const int overworldCaveDepth = 160;
    if (settings.noiseCaves)
    {
        m_stages.push_back(std::make_unique<NoiseCaveCarver>(seed, false));
    }

    if (settings.caves)
    {
        m_stages.push_back(std::make_unique<LargeCaveFeature>(seed, overworldCaveDepth));
    }

    if (settings.ravines)
    {
        m_stages.push_back(std::make_unique<RavineFeature>(seed, overworldCaveDepth));
    }

    if (settings.caveBiomes)
    {
        m_stages.push_back(std::make_unique<CaveBiomeStage>(this));
        m_stages.push_back(std::make_unique<GlacierStage>(this));
    }

    if (settings.lakes || settings.lavaLakes)
    {
        m_stages.push_back(std::make_unique<LakeStage>(this));
    }

    if (settings.stoneVariants)
    {
        m_stages.push_back(std::make_unique<StoneVariantStage>(this));
    }

    m_stages.push_back(std::make_unique<BadlandsStage>(this));
    m_stages.push_back(std::make_unique<ClayStage>(this));

    if (settings.ores)
    {
        m_stages.push_back(std::make_unique<OreStage>(this));
    }

    if (settings.caveBiomes)
    {
        m_stages.push_back(std::make_unique<DripstoneStage>(this));
        m_stages.push_back(std::make_unique<IceCaveStage>(this));
        m_stages.push_back(std::make_unique<DesertCaveStage>(this));
        m_stages.push_back(std::make_unique<OvergrownHollowsStage>(this));
        m_stages.push_back(std::make_unique<MagmaDepthsStage>(this));
    }

    if (settings.hotDepths)
    {
        m_stages.push_back(std::make_unique<HotDepthStage>(this));
    }

    if (settings.springs)
    {
        m_stages.push_back(std::make_unique<SpringStage>(this, Block::ByName("water_flow"), false));
        m_stages.push_back(std::make_unique<SpringStage>(this, Block::ByName("lava_flow"), true));
    }

    m_stages.push_back(std::make_unique<DungeonStage>(this));

    if (settings.vegetation)
    {
        m_stages.push_back(std::make_unique<ForestRockStage>(this));
    }

    if (settings.trees)
    {
        m_stages.push_back(std::make_unique<TreeStage>(this));
    }

    if (settings.vegetation)
    {
        m_stages.push_back(std::make_unique<VegetationStage>(this));
        m_stages.push_back(std::make_unique<SugarCaneStage>(this));
        m_stages.push_back(std::make_unique<LilyPadStage>(this));
        m_stages.push_back(std::make_unique<JungleStage>(this));
    }

    if (settings.desertPlants)
    {
        m_stages.push_back(std::make_unique<DesertPlantStage>(this));
        m_stages.push_back(std::make_unique<SandLayerStage>(this));
    }

    if (settings.iceSpikes)
    {
        m_stages.push_back(std::make_unique<IceSpikeStage>(this));
    }

    if (settings.snow)
    {
        m_stages.push_back(std::make_unique<SnowStage>(this));
    }

    m_stages.push_back(std::make_unique<LavaHardenStage>());
}

void TerrainGenerator::GetHeights(double *buffer, bool *dryCells, int chunkX, int chunkZ)
{
    static const int blendWidth = s_gridX + 5;

    Biome *biomeGrid[blendWidth * blendWidth];
    double depthGrid[blendWidth * blendWidth];
    for (int xi = 0; xi < blendWidth; xi++)
    {
        for (int zi = 0; zi < blendWidth; zi++)
        {
            int levelX                      = chunkX * Chunk::s_sizeX + (xi - 2) * s_chunkWidth;
            int levelZ                      = chunkZ * Chunk::s_sizeZ + (zi - 2) * s_chunkWidth;
            biomeGrid[xi + zi * blendWidth] = m_biomeSource.GetBiome(levelX, levelZ);
            depthGrid[xi + zi * blendWidth] = GetBiomeDepth(biomeGrid[xi + zi * blendWidth], levelX, levelZ);
        }
    }

    for (int xx = 0; xx < s_gridX; xx++)
    {
        for (int zz = 0; zz < s_gridZ; zz++)
        {
            ComputeDensityColumn(chunkX * (s_gridX - 1) + xx, chunkZ * (s_gridZ - 1) + zz, &biomeGrid[xx + zz * blendWidth],
                                 &depthGrid[xx + zz * blendWidth], blendWidth, &buffer[(xx * s_gridZ + zz) * s_gridY]);
        }
    }

    // Dry basins stay dry only away from other biomes, otherwise rivers and coasts get walled off by stone
    for (int xc = 0; xc < s_gridX - 1; xc++)
    {
        for (int zc = 0; zc < s_gridZ - 1; zc++)
        {
            bool dry = true;
            for (int xi = xc; xi <= xc + 5 && dry; xi++)
            {
                for (int zi = zc; zi <= zc + 5 && dry; zi++)
                {
                    dry = biomeGrid[xi + zi * blendWidth]->HasDryBasins();
                }
            }

            dryCells[xc * (s_gridZ - 1) + zc] = dry;
        }
    }
}

int64_t TerrainGenerator::GetDensityKey(int gridX, int gridZ)
{
    return (int64_t) (((uint64_t) (uint32_t) gridX << 32) | (uint32_t) gridZ);
}

double TerrainGenerator::GetBiomeDepth(Biome *biome, int levelX, int levelZ) const
{
    if (biome->HasContinentalDepth())
    {
        return m_biomeSource.GetOceanDepth(m_biomeSource.GetContinentalness(levelX, levelZ));
    }

    if (biome->GetMountainDepth() > 0.0f)
    {
        return biome->GetDepth() + biome->GetMountainDepth() * m_biomeSource.GetMountainFactor(levelX, levelZ);
    }

    return biome->GetDepth();
}

const double *TerrainGenerator::GetDensityColumn(int gridX, int gridZ)
{
    int64_t key = GetDensityKey(gridX, gridZ);
    auto it     = m_densityCache.find(key);
    if (it != m_densityCache.end())
    {
        return it->second.data();
    }

    Biome *biomes[5 * 5];
    double depths[5 * 5];
    for (int xb = 0; xb < 5; xb++)
    {
        for (int zb = 0; zb < 5; zb++)
        {
            int levelX          = (gridX + xb - 2) * s_chunkWidth;
            int levelZ          = (gridZ + zb - 2) * s_chunkWidth;
            biomes[xb + zb * 5] = m_biomeSource.GetBiome(levelX, levelZ);
            depths[xb + zb * 5] = GetBiomeDepth(biomes[xb + zb * 5], levelX, levelZ);
        }
    }

    std::array<double, s_gridY> &column = m_densityCache[key];
    ComputeDensityColumn(gridX, gridZ, biomes, depths, 5, column.data());
    return column.data();
}

int TerrainGenerator::GetTerrainHeight(int levelX, int levelZ)
{
    if (m_settings.worldType == eWorldType_Flat)
    {
        return m_flatHeight - 1;
    }

    int gridX = (int) floor((double) levelX / s_chunkWidth);
    int gridZ = (int) floor((double) levelZ / s_chunkWidth);
    double fx = (double) (levelX - gridX * s_chunkWidth) / s_chunkWidth;
    double fz = (double) (levelZ - gridZ * s_chunkWidth) / s_chunkWidth;

    // Grid aligned samples skip the neighbor columns since their weight is zero anyway
    const double *c00 = GetDensityColumn(gridX, gridZ);
    const double *c10 = fx > 0.0 ? GetDensityColumn(gridX + 1, gridZ) : c00;
    const double *c01 = fz > 0.0 ? GetDensityColumn(gridX, gridZ + 1) : c00;
    const double *c11 = fx > 0.0 && fz > 0.0 ? GetDensityColumn(gridX + 1, gridZ + 1) : (fx > 0.0 ? c10 : c01);

    for (int yc = s_gridY - 2; yc >= 0; yc--)
    {
        double low  = (c00[yc] * (1.0 - fx) + c10[yc] * fx) * (1.0 - fz) + (c01[yc] * (1.0 - fx) + c11[yc] * fx) * fz;
        double high = (c00[yc + 1] * (1.0 - fx) + c10[yc + 1] * fx) * (1.0 - fz) + (c01[yc + 1] * (1.0 - fx) + c11[yc + 1] * fx) * fz;
        for (int y = s_chunkHeight - 1; y >= 0; y--)
        {
            if (low + (high - low) * y / s_chunkHeight > 0.0)
            {
                return s_terrainBase + yc * s_chunkHeight + y;
            }
        }
    }

    return s_terrainBase - 1;
}

Block *TerrainGenerator::GetSurfaceBlock(Biome *biome, int levelX, int levelZ, int y) const
{
    Block *top        = biome->GetTopBlock();
    double depthValue = m_surfaceNoise.GetValue((double) levelX / 16.0, (double) levelZ / 16.0);
    if (y >= s_seaLevel - 1)
    {
        if (biome->HasPodzolSurface() && depthValue > -0.95)
        {
            return Block::ByName(depthValue > 1.75 ? "coarse_dirt" : "podzol");
        }

        return top;
    }

    if (y < s_seaLevel - 7 - std::max((int) (depthValue / 3.0 + 3.0), 1))
    {
        return depthValue > 2.0 ? Block::ByName("sand") : Block::ByName("gravel");
    }

    return depthValue > 0.0 ? Block::ByName("sand") : biome->GetFillerBlock();
}

void TerrainGenerator::ClearDensityCache()
{
    m_densityCache.clear();
}

void TerrainGenerator::ComputeDensityColumn(int gridX, int gridZ, Biome *const *biomes, const double *depths, int stride, double *out)
{
    double centerDepth = depths[2 + 2 * stride];

    double sumScale  = 0.0;
    double sumDepth  = 0.0;
    double sumWeight = 0.0;
    for (int xb = -2; xb <= 2; xb++)
    {
        for (int zb = -2; zb <= 2; zb++)
        {
            double biomeDepth = depths[(xb + 2) + (zb + 2) * stride];
            double biomeScale = biomes[(xb + 2) + (zb + 2) * stride]->GetScale();
            bool deeper       = biomeDepth > centerDepth;
            if (m_settings.worldType == eWorldType_Amplified && biomeDepth > 0.0)
            {
                biomeDepth = 1.0 + biomeDepth * 2.0;
                biomeScale = 1.0 + biomeScale * 4.0;
            }

            double weight = m_blendWeights[(xb + 2) + (zb + 2) * 5] / (biomeDepth + 2.0);
            if (deeper)
            {
                weight /= 2.0;
            }

            sumScale += biomeScale * weight;
            sumDepth += biomeDepth * weight;
            sumWeight += weight;
        }
    }

    double scale = sumScale / sumWeight;
    double depth = sumDepth / sumWeight;
    scale        = scale * 0.9 + 0.1;
    depth        = (depth * 4.0 - 1.0) / 8.0;

    double rdepth = m_depthNoise.GetValue((double) gridX * m_settings.depthNoiseScale, (double) gridZ * m_settings.depthNoiseScale) / 8000.0;
    if (rdepth < 0.0)
    {
        rdepth = -rdepth * 0.3;
    }
    rdepth = rdepth * 3.0 - 2.0;
    if (rdepth < 0.0)
    {
        rdepth /= 2.0;
        if (rdepth < -1.0)
        {
            rdepth = -1.0;
        }
        rdepth /= 1.4;
        rdepth /= 2.0;
    }
    else
    {
        if (rdepth > 1.0)
        {
            rdepth = 1.0;
        }
        rdepth /= 8.0;
    }

    const double centerCell = 8.5;

    for (int yy = 0; yy < s_gridY; yy++)
    {
        double cellDepth = (depth + rdepth * 0.2) * centerCell / 8.0;

        double yCenter = centerCell + cellDepth * 4.0;
        double yOffs   = ((double) yy - yCenter) * 12.0 * 128.0 / 256.0 / scale;
        if (yOffs < 0.0)
        {
            yOffs *= 4.0;
        }

        double gridXz = (double) gridX;
        double gridZz = (double) gridZ;

        double lower    = m_lowerNoise.GetValue(gridXz * m_settings.coordinateScale, (double) yy * m_settings.heightScale,
                                                gridZz * m_settings.coordinateScale) /
                          512.0;
        double upper    = m_upperNoise.GetValue(gridXz * m_settings.coordinateScale, (double) yy * m_settings.heightScale,
                                                gridZz * m_settings.coordinateScale) /
                          512.0;
        double selector = (m_selectorNoise.GetValue(gridXz * m_settings.coordinateScale / 80.0, (double) yy * m_settings.heightScale / 160.0,
                                                    gridZz * m_settings.coordinateScale / 80.0) /
                                   10.0 +
                           1.0) /
                          2.0;

        double value;
        if (selector < 0.0)
        {
            value = lower;
        }
        else if (selector > 1.0)
        {
            value = upper;
        }
        else
        {
            value = lower + (upper - lower) * selector;
        }

        value -= yOffs;

        if (yy > s_gridY - 4)
        {
            double slide = (double) (yy - (s_gridY - 4)) / 3.0;
            value        = value * (1.0 - slide) + -10.0 * slide;
        }

        out[yy] = value;
    }
}

void TerrainGenerator::PrepareHeights(Chunk &chunk, const double *buffer, const bool *dryCells)
{
    Block *stone = Block::ByName("stone");
    Block *water = Block::ByName("water_still");

    int xChunks = Chunk::s_sizeX / s_chunkWidth;
    int zChunks = Chunk::s_sizeZ / s_chunkWidth;
    int yChunks = s_densityDepth / s_chunkHeight;

    // Terrain sits on a solid base so the density field keeps its shape while the deep rock gains room for caves
    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int y = 0; y < s_terrainBase; y++)
            {
                chunk.SetBlock(x, y, z, stone);
            }
        }
    }

    for (int xc = 0; xc < xChunks; xc++)
    {
        for (int zc = 0; zc < zChunks; zc++)
        {
            for (int yc = 0; yc < yChunks; yc++)
            {
                double yStep = 1.0 / (double) s_chunkHeight;

                double s0 = buffer[(xc * s_gridZ + zc) * s_gridY + yc];
                double s1 = buffer[(xc * s_gridZ + (zc + 1)) * s_gridY + yc];
                double s2 = buffer[((xc + 1) * s_gridZ + zc) * s_gridY + yc];
                double s3 = buffer[((xc + 1) * s_gridZ + (zc + 1)) * s_gridY + yc];

                double s0a = (buffer[(xc * s_gridZ + zc) * s_gridY + (yc + 1)] - s0) * yStep;
                double s1a = (buffer[(xc * s_gridZ + (zc + 1)) * s_gridY + (yc + 1)] - s1) * yStep;
                double s2a = (buffer[((xc + 1) * s_gridZ + zc) * s_gridY + (yc + 1)] - s2) * yStep;
                double s3a = (buffer[((xc + 1) * s_gridZ + (zc + 1)) * s_gridY + (yc + 1)] - s3) * yStep;

                for (int y = 0; y < s_chunkHeight; y++)
                {
                    double xStep = 1.0 / (double) s_chunkWidth;
                    double c0    = s0;
                    double c1    = s1;
                    double c0a   = (s2 - s0) * xStep;
                    double c1a   = (s3 - s1) * xStep;

                    for (int x = 0; x < s_chunkWidth; x++)
                    {
                        double zStep     = 1.0 / (double) s_chunkWidth;
                        double value     = c0;
                        double valueStep = (c1 - c0) * zStep;
                        value -= valueStep;

                        for (int z = 0; z < s_chunkWidth; z++)
                        {
                            value += valueStep;

                            int localX = xc * s_chunkWidth + x;
                            int localY = s_terrainBase + yc * s_chunkHeight + y;
                            int localZ = zc * s_chunkWidth + z;

                            if (value > 0.0)
                            {
                                chunk.SetBlock(localX, localY, localZ, stone);
                            }
                            else if (localY < s_seaLevel)
                            {
                                chunk.SetBlock(localX, localY, localZ, dryCells[xc * zChunks + zc] ? stone : water);
                            }
                        }

                        c0 += c0a;
                        c1 += c1a;
                    }

                    s0 += s0a;
                    s1 += s1a;
                    s2 += s2a;
                    s3 += s3a;
                }
            }
        }
    }
}

void TerrainGenerator::BuildSurfaces(Chunk &chunk, const ChunkPos &pos)
{
    Block *bedrock    = Block::ByName("bedrock");
    Block *stone      = Block::ByName("stone");
    Block *air        = Block::ByName("air");
    Block *water      = Block::ByName("water_still");
    Block *sand       = Block::ByName("sand");
    Block *gravel     = Block::ByName("gravel");
    Block *sandstone  = Block::ByName("sandstone");
    Block *podzol     = Block::ByName("podzol");
    Block *coarseDirt = Block::ByName("coarse_dirt");

    const int deepFloorOffset   = 7;
    const double sandPatchNoise = 2.0;
    const double surfaceScale   = 1.0 / 16.0;

    Random random(GetChunkSeed(pos, 0x9e3779b97f4a7c15ULL));

    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            int levelX = pos.x * Chunk::s_sizeX + x;
            int levelZ = pos.z * Chunk::s_sizeZ + z;

            Biome *biome = chunk.GetSurfaceBiome(x, z);

            double depthValue = m_surfaceNoise.GetValue((double) levelX * surfaceScale, (double) levelZ * surfaceScale);
            int runDepth      = std::max((int) (depthValue / 3.0 + 3.0 + random.NextDouble() * 0.25), 1);

            Block *surfaceTop = biome ? biome->GetTopBlock() : stone;
            if (biome && biome->HasPodzolSurface() && depthValue > -0.95)
            {
                surfaceTop = depthValue > 1.75 ? coarseDirt : podzol;
            }

            int run            = -1;
            Block *top         = surfaceTop;
            Block *material    = biome ? biome->GetFillerBlock() : stone;
            Block *runMaterial = material;

            for (int y = Chunk::s_sizeY - 1; y >= 0; y--)
            {
                if (y <= random.NextInt(0, 4))
                {
                    chunk.SetBlock(x, y, z, bedrock);
                    continue;
                }

                Block *old = Block::ById(chunk.GetBlockId(x, y, z));
                if (old == air)
                {
                    run = -1;
                }
                else if (old == stone)
                {
                    if (run == -1)
                    {
                        if (y >= s_seaLevel - 4 && y <= s_seaLevel + 1)
                        {
                            top      = surfaceTop;
                            material = biome ? biome->GetFillerBlock() : stone;
                        }

                        if (y < s_seaLevel && top == air)
                        {
                            top = water;
                        }

                        run         = runDepth;
                        runMaterial = material;
                        if (y < s_seaLevel - 1)
                        {
                            if (y < s_seaLevel - deepFloorOffset - runDepth)
                            {
                                runMaterial = depthValue > sandPatchNoise ? sand : gravel;
                            }
                            else if (depthValue > 0.0)
                            {
                                runMaterial = sand;
                            }
                        }

                        chunk.SetBlock(x, y, z, y >= s_seaLevel - 1 ? top : runMaterial);
                    }
                    else if (run > 0)
                    {
                        run--;
                        chunk.SetBlock(x, y, z, runMaterial);
                        if (run == 0 && runMaterial == sand)
                        {
                            run         = random.NextInt(0, 3);
                            runMaterial = sandstone;
                        }
                    }
                }
            }
        }
    }
}

void TerrainGenerator::BuildFlatChunk(Chunk &chunk, const ChunkPos &pos)
{
    Biome *biome = m_biomeSource.GetBiome(0, 0);
    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            chunk.SetColumnBiome(x, z, biome);
        }
    }

    int y = 0;
    for (size_t i = 0; i < m_settings.flatLayers.size() && y < Chunk::s_sizeY; i++)
    {
        Block *block = Block::ByName(m_settings.flatLayers[i].block);
        for (int layer = 0; layer < m_settings.flatLayers[i].height && y < Chunk::s_sizeY; layer++, y++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                for (int z = 0; z < Chunk::s_sizeZ; z++)
                {
                    chunk.SetBlock(x, y, z, block);
                }
            }
        }
    }

    if (m_flatVoid && pos.x == 0 && pos.z == 0)
    {
        Block *stone = Block::ByName("stone");
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                chunk.SetBlock(x, m_flatHeight - 1, z, stone);
            }
        }
    }

    for (size_t i = 0; i < m_stages.size(); i++)
    {
        m_stages[i]->Apply(chunk, pos);
    }
}

void TerrainGenerator::BuildCaveChunk(Chunk &chunk, const ChunkPos &pos)
{
    Biome *cave    = Biome::ByName("cave");
    Block *stone   = Block::ByName("stone");
    Block *bedrock = Block::ByName("bedrock");
    Random random(GetChunkSeed(pos, 0x9e3779b97f4a7c15ULL));
    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            chunk.SetColumnBiome(x, z, cave);
            int floor   = random.NextInt(0, 4);
            int ceiling = s_caveCeiling - random.NextInt(0, 4);
            for (int y = 0; y <= s_caveCeiling; y++)
            {
                chunk.SetBlock(x, y, z, y <= floor || y >= ceiling ? bedrock : stone);
            }
        }
    }

    for (size_t i = 0; i < m_stages.size(); i++)
    {
        m_stages[i]->Apply(chunk, pos);
    }
}

uint64_t TerrainGenerator::GetChunkSeed(const ChunkPos &pos, uint64_t salt) const
{
    return (((uint64_t) (uint32_t) pos.x * 341873128712ULL + (uint64_t) (uint32_t) pos.z * 132897987541ULL)) ^ ((uint64_t) m_seed * salt);
}

uint32_t TerrainGenerator::GetSeed() const
{
    return m_seed;
}

const BiomeSource &TerrainGenerator::GetBiomeSource() const
{
    return m_biomeSource;
}

const WorldGenSettings &TerrainGenerator::GetSettings() const
{
    return m_settings;
}

const LakeFeature &TerrainGenerator::GetLakeFeature() const
{
    return m_lakeFeature;
}

void TerrainGenerator::GetLakePlan(const ChunkPos &pos, LakeFeature::Plan *plan)
{
    plan->active = false;
    if (!m_settings.lakes)
    {
        return;
    }

    Random random(GetChunkSeed(pos, 0x9e3779b97f4a7c15ULL ^ 0x1a4eULL));
    if (random.NextInt(0, m_settings.lakeChance - 1) != 0)
    {
        return;
    }

    int centerX = pos.x * Chunk::s_sizeX + Chunk::s_sizeX / 2;
    int centerZ = pos.z * Chunk::s_sizeZ + Chunk::s_sizeZ / 2;
    if (!m_biomeSource.GetBiome(centerX, centerZ)->IsDecorated())
    {
        return;
    }

    int y       = random.NextInt(0, s_genDepth - 1);
    int surface = GetTerrainHeight(centerX, centerZ);
    if (y > surface)
    {
        y = surface;
    }

    if (y <= 4)
    {
        return;
    }

    m_lakeFeature.BuildPlan(random, y - LakeFeature::s_height / 2, plan);
}

bool TerrainGenerator::IsStone(Block *block)
{
    static Block *stone    = Block::ByName("stone");
    static Block *granite  = Block::ByName("granite");
    static Block *diorite  = Block::ByName("diorite");
    static Block *andesite = Block::ByName("andesite");
    return block == stone || block == granite || block == diorite || block == andesite;
}

int TerrainGenerator::FindTopSolid(const Chunk &chunk, int x, int z)
{
    for (int y = Chunk::s_sizeY - 2; y >= 1; y--)
    {
        Block *block = Block::ById(chunk.GetBlockId(x, y, z));
        if (block && block->IsSolid())
        {
            return y;
        }
    }

    return -1;
}

BlockPos TerrainGenerator::FindSpawn()
{
    if (m_settings.worldType == eWorldType_Flat)
    {
        return BlockPos(Chunk::s_sizeX / 2, m_flatHeight, Chunk::s_sizeZ / 2);
    }

    if (m_settings.worldType == eWorldType_Caves)
    {
        return BlockPos(Chunk::s_sizeX / 2, s_caveCeiling / 2, Chunk::s_sizeZ / 2);
    }

    const int biomeSearchRadius = 256;
    const int biomeSearchStep   = 8;
    const int spawnAttempts     = 1000;
    const int spawnStep         = 64;

    const int searchCenterRange = 4096;

    Random random(m_seed);

    // Biome noise is zero at the origin, so we center the search somewhere seeded or every world would spawn in the same biome
    int centerX      = random.NextInt(0, searchCenterRange * 2) - searchCenterRange;
    int centerZ      = random.NextInt(0, searchCenterRange * 2) - searchCenterRange;
    int spawnX       = centerX;
    int spawnZ       = centerZ;
    int64_t bestDist = -1;
    for (int x = -biomeSearchRadius; x <= biomeSearchRadius; x += biomeSearchStep)
    {
        for (int z = -biomeSearchRadius; z <= biomeSearchRadius; z += biomeSearchStep)
        {
            int64_t dist = (int64_t) x * x + (int64_t) z * z;
            if ((bestDist < 0 || dist < bestDist) && m_biomeSource.GetBiome(centerX + x, centerZ + z)->IsDecorated())
            {
                bestDist = dist;
                spawnX   = centerX + x;
                spawnZ   = centerZ + z;
            }
        }
    }

    int surface = GetTerrainHeight(spawnX, spawnZ);
    for (int attempt = 0; attempt < spawnAttempts; attempt++)
    {
        if (m_biomeSource.GetBiome(spawnX, spawnZ)->IsDecorated() && surface >= s_seaLevel)
        {
            break;
        }

        spawnX += random.NextInt(0, spawnStep - 1) - random.NextInt(0, spawnStep - 1);
        spawnZ += random.NextInt(0, spawnStep - 1) - random.NextInt(0, spawnStep - 1);
        surface = GetTerrainHeight(spawnX, spawnZ);
    }

    return BlockPos(spawnX, surface + 1, spawnZ);
}

void TerrainGenerator::GenerateChunk(Chunk &chunk, const ChunkPos &pos)
{
    if (m_settings.worldType == eWorldType_Flat)
    {
        BuildFlatChunk(chunk, pos);
        return;
    }

    if (m_settings.worldType == eWorldType_Caves)
    {
        BuildCaveChunk(chunk, pos);
        return;
    }

    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            chunk.SetColumnBiome(x, z, m_biomeSource.GetBiome(pos.x * Chunk::s_sizeX + x, pos.z * Chunk::s_sizeZ + z));
        }
    }

    double buffer[s_gridX * s_gridY * s_gridZ];
    bool dryCells[(s_gridX - 1) * (s_gridZ - 1)];
    GetHeights(buffer, dryCells, pos.x, pos.z);
    PrepareHeights(chunk, buffer, dryCells);
    BuildSurfaces(chunk, pos);

    // We keep density columns across chunks since they only depend on their grid position
    if (m_densityCache.size() > 8192)
    {
        m_densityCache.clear();
    }

    for (int xx = 0; xx < s_gridX; xx++)
    {
        for (int zz = 0; zz < s_gridZ; zz++)
        {
            std::array<double, s_gridY> &column = m_densityCache[GetDensityKey(pos.x * (s_gridX - 1) + xx, pos.z * (s_gridZ - 1) + zz)];
            std::copy(&buffer[(xx * s_gridZ + zz) * s_gridY], &buffer[(xx * s_gridZ + zz) * s_gridY] + s_gridY, column.begin());
        }
    }

    for (size_t i = 0; i < m_stages.size(); i++)
    {
        m_stages[i]->Apply(chunk, pos);
    }
}
