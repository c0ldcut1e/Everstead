#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../block/BlockPos.h"
#include "../chunk/Chunk.h"
#include "BiomeSource.h"
#include "LakeFeature.h"
#include "PerlinNoise.h"
#include "WorldGenSettings.h"
#include "WorldGenStage.h"

class TerrainGenerator
{
public:
    TerrainGenerator(uint32_t seed, const WorldGenSettings &settings);

    void GenerateChunk(Chunk &chunk, const ChunkPos &pos);
    BlockPos FindSpawn();

    int GetTerrainHeight(int levelX, int levelZ);
    Block *GetSurfaceBlock(Biome *biome, int levelX, int levelZ, int y) const;
    void ClearDensityCache();
    void GetLakePlan(const ChunkPos &pos, LakeFeature::Plan *plan);
    uint64_t GetChunkSeed(const ChunkPos &pos, uint64_t salt) const;
    uint32_t GetSeed() const;
    const BiomeSource &GetBiomeSource() const;
    const WorldGenSettings &GetSettings() const;
    const LakeFeature &GetLakeFeature() const;

    static bool IsStone(Block *block);
    static int FindTopSolid(const Chunk &chunk, int x, int z);

    static const int s_terrainBase  = 32;
    static const int s_densityDepth = 224;
    static const int s_seaLevel     = 64 + s_terrainBase;
    static const int s_genDepth     = 128 + s_terrainBase;
    static const int s_caveCeiling  = Chunk::s_sizeY - 1;
    static const int s_chunkWidth   = 4;
    static const int s_chunkHeight  = 8;
    static const int s_gridX        = Chunk::s_sizeX / s_chunkWidth + 1;
    static const int s_gridZ        = Chunk::s_sizeZ / s_chunkWidth + 1;
    static const int s_gridY        = s_densityDepth / s_chunkHeight + 1;

private:
    void GetHeights(double *buffer, bool *dryCells, int chunkX, int chunkZ);
    void ComputeDensityColumn(int gridX, int gridZ, Biome *const *biomes, const double *depths, int stride, double *out);
    double GetBiomeDepth(Biome *biome, int levelX, int levelZ) const;
    static int64_t GetDensityKey(int gridX, int gridZ);
    const double *GetDensityColumn(int gridX, int gridZ);
    void PrepareHeights(Chunk &chunk, const double *buffer, const bool *dryCells);
    void BuildSurfaces(Chunk &chunk, const ChunkPos &pos);
    void BuildFlatChunk(Chunk &chunk, const ChunkPos &pos);
    void BuildCaveChunk(Chunk &chunk, const ChunkPos &pos);

    uint32_t m_seed;
    WorldGenSettings m_settings;
    Random m_noiseRandom;
    BiomeSource m_biomeSource;
    PerlinNoise m_lowerNoise;
    PerlinNoise m_upperNoise;
    PerlinNoise m_selectorNoise;
    PerlinNoise m_surfaceNoise;
    PerlinNoise m_depthNoise;
    LakeFeature m_lakeFeature;
    int m_flatHeight;
    bool m_flatVoid;
    double m_blendWeights[5 * 5];
    std::unordered_map<int64_t, std::array<double, s_gridY>> m_densityCache;
    std::vector<std::unique_ptr<WorldGenStage>> m_stages;
};
