#include "BiomeSource.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

const float BiomeSource::s_continentScale     = 1.0f / 4096.0f;
const float BiomeSource::s_biomeScale         = 1.0f / 1024.0f;
const float BiomeSource::s_oceanCutoff        = -0.3f;
const float BiomeSource::s_desertCutoff       = 0.25f;
const float BiomeSource::s_desertTemperature  = 0.05f;
const float BiomeSource::s_forestCutoff       = 0.0f;
const float BiomeSource::s_temperatureScale   = 1.0f / 2048.0f;
const float BiomeSource::s_spruceCutoff       = -0.08f;
const float BiomeSource::s_birchCutoff        = 0.2f;
const float BiomeSource::s_swampCutoff        = -0.22f;
const float BiomeSource::s_savannaCutoff      = 0.08f;
const float BiomeSource::s_savannaTemperature = 0.1f;
const float BiomeSource::s_snowTemperature    = -0.16f;
const float BiomeSource::s_beachWidth         = 0.004f;
const float BiomeSource::s_shelfWidth         = 0.04f;
const float BiomeSource::s_shoreDepth         = -0.2f;
const float BiomeSource::s_warpFrequency      = 1.0f / 384.0f;
const float BiomeSource::s_warpAmplitude      = 96.0f;
const float BiomeSource::s_edgeWarpFrequency  = 1.0f / 48.0f;
const float BiomeSource::s_edgeWarpAmplitude  = 28.0f;
const float BiomeSource::s_mountainScale      = 1.0f / 1536.0f;
const float BiomeSource::s_mountainCutoff     = 0.3f;
const float BiomeSource::s_mountainInland     = -0.15f;
const float BiomeSource::s_iceSpikeScale      = 1.0f / 512.0f;
const float BiomeSource::s_iceSpikeCutoff     = 0.2f;
const float BiomeSource::s_meadowBand         = 0.12f;
const float BiomeSource::s_jungleTemperature  = 0.28f;
const float BiomeSource::s_jungleEdgeBand     = 0.06f;
const float BiomeSource::s_bambooCutoff       = 0.25f;
const float BiomeSource::s_oldGrowthCutoff    = -0.2f;
const int BiomeSource::s_regionCellSize       = 32;
const int BiomeSource::s_regionRadius         = 3;
const int BiomeSource::s_regionMinCells       = 12;

BiomeSource::BiomeSource(uint32_t seed, const WorldGenSettings &settings) : m_riverLayer(seed)
{
    m_seed         = seed;
    m_zoom         = settings.worldType == eWorldType_LargeBiomes ? 4.0f : 1.0f;
    m_fixedBiome   = settings.worldType == eWorldType_Flat || settings.worldType == eWorldType_SingleBiome ? Biome::ByName(settings.biome) : nullptr;
    m_ocean        = Biome::ByName("ocean");
    m_plains       = Biome::ByName("plains");
    m_desert       = Biome::ByName("desert");
    m_beach        = Biome::ByName("beach");
    m_forest       = Biome::ByName("forest");
    m_spruceForest = Biome::ByName("spruce_forest");
    m_birchForest  = Biome::ByName("birch_forest");
    m_savanna      = Biome::ByName("savanna");
    m_snowyTundra  = Biome::ByName("snowy_tundra");
    m_mountains    = Biome::ByName("mountains");
    m_river        = Biome::ByName("river");
    m_frozenRiver  = Biome::ByName("frozen_river");
    m_iceSpikes    = Biome::ByName("ice_spikes");
    m_glacier      = Biome::ByName("glacier");
    m_cherryGrove  = Biome::ByName("cherry_grove");
    m_meadow       = Biome::ByName("meadow");
    m_swamp        = Biome::ByName("swamp");
    m_jungle       = Biome::ByName("jungle");
    m_jungleEdge   = Biome::ByName("jungle_edge");
    m_bambooJungle = Biome::ByName("bamboo_jungle");
    m_oldGrowthSpruceTaiga = Biome::ByName("old_growth_spruce_taiga");
    m_badlands             = Biome::ByName("badlands");
    m_woodedBadlands       = Biome::ByName("wooded_badlands");
    m_darkForest           = Biome::ByName("dark_forest");
    m_stonyShore           = Biome::ByName("stony_shore");

    m_continent.SetSeed((int) seed ^ 0x4f6e);
    m_continent.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_continent.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_continent.SetFractalOctaves(4);
    m_continent.SetFrequency(1.0f);

    m_island.SetSeed((int) seed ^ 0x15a7);
    m_island.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_island.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_island.SetFractalOctaves(3);
    m_island.SetFrequency(1.0f);

    m_biome.SetSeed((int) seed ^ 0x7e3a);
    m_biome.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_biome.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_biome.SetFractalOctaves(6);
    m_biome.SetFrequency(1.0f);

    m_temperature.SetSeed((int) seed ^ 0x51c3);
    m_temperature.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_temperature.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_temperature.SetFractalOctaves(5);
    m_temperature.SetFrequency(1.0f);

    m_mountain.SetSeed((int) seed ^ 0x6b17);
    m_mountain.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_mountain.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_mountain.SetFractalOctaves(4);
    m_mountain.SetFrequency(1.0f);

    m_iceSpike.SetSeed((int) seed ^ 0x1ce5);
    m_iceSpike.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_iceSpike.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_iceSpike.SetFractalOctaves(3);
    m_iceSpike.SetFrequency(1.0f);

    m_warp.SetSeed((int) seed ^ 0x2d91);
    m_warp.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    m_warp.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
    m_warp.SetFractalOctaves(4);
    m_warp.SetFrequency(s_warpFrequency);
    m_warp.SetDomainWarpAmp(s_warpAmplitude);

    m_edgeWarp.SetSeed((int) seed ^ 0x3e7b);
    m_edgeWarp.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    m_edgeWarp.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
    m_edgeWarp.SetFractalOctaves(2);
    m_edgeWarp.SetFrequency(s_edgeWarpFrequency);
    m_edgeWarp.SetDomainWarpAmp(s_edgeWarpAmplitude);
}

void BiomeSource::Warp(int levelX, int levelZ, float *outX, float *outZ) const
{
    float x = (float) levelX / m_zoom;
    float z = (float) levelZ / m_zoom;
    m_warp.DomainWarp(x, z);
    *outX = x;
    *outZ = z;
}

// Islands lift the ocean floor and fade out toward land, so the value stays continuous across coasts
float BiomeSource::GetContinent(float x, float z) const
{
    constexpr float kIslandScale  = 1.0f / 288.0f;
    constexpr float kIslandCutoff = 0.3f;
    constexpr float kIslandLift   = 1.3f;
    constexpr float kIslandFade   = 0.1f;
    float continent               = m_continent.GetNoise(x * s_continentScale, z * s_continentScale);
    float fade                    = std::clamp((s_oceanCutoff + kIslandFade * 0.5f - continent) / kIslandFade, 0.0f, 1.0f);
    float island                  = std::max(0.0f, m_island.GetNoise(x * kIslandScale, z * kIslandScale) - kIslandCutoff);
    return continent + island * kIslandLift * fade * fade * (3.0f - 2.0f * fade);
}

Biome *BiomeSource::GetBiome(int levelX, int levelZ) const
{
    if (m_fixedBiome)
    {
        return m_fixedBiome;
    }

    float x = 0.0f;
    float z = 0.0f;
    Warp(levelX, levelZ, &x, &z);
    float continent = GetContinent(x, z);
    if (continent < s_oceanCutoff)
    {
        return m_ocean;
    }

    float biome       = m_biome.GetNoise(x * s_biomeScale, z * s_biomeScale);
    float temperature = m_temperature.GetNoise(x * s_temperatureScale, z * s_temperatureScale);
    Biome *land       = FilterSmallRegion(levelX, levelZ, GetLandBiome(x, z, continent, biome, temperature));
    if (land != m_desert && land != m_badlands && land != m_woodedBadlands && continent < s_oceanCutoff + s_beachWidth)
    {
        return m_mountain.GetNoise(x * s_mountainScale, z * s_mountainScale) > s_mountainCutoff - s_meadowBand ? m_stonyShore : m_beach;
    }

    if (m_riverLayer.IsRiver((int) std::floor(levelX / m_zoom), (int) std::floor(levelZ / m_zoom)))
    {
        return land == m_snowyTundra || land == m_iceSpikes || land == m_glacier ? m_frozenRiver : m_river;
    }

    return land;
}

Biome *BiomeSource::GetLandBiome(float x, float z, float continent, float biome, float temperature) const
{
    if (biome > s_desertCutoff && temperature > s_desertTemperature)
    {
        float badlands = m_iceSpike.GetNoise(x * s_iceSpikeScale, z * s_iceSpikeScale);
        if (badlands <= s_bambooCutoff)
        {
            return m_desert;
        }

        // The plateau stays well inside badlands so its raised terrain never meets another biome's edge
        const float plateauMargin = 0.1f;
        bool plateau              = biome > s_desertCutoff + plateauMargin && temperature > s_desertTemperature + plateauMargin &&
                                    badlands > s_bambooCutoff + plateauMargin && m_mountain.GetNoise(x * s_mountainScale, z * s_mountainScale) > 0.0f;
        return plateau ? m_woodedBadlands : m_badlands;
    }

    // This keeps mountains away from the coast so they don't rise straight out of the sea
    float mountain = m_mountain.GetNoise(x * s_mountainScale, z * s_mountainScale);
    if (continent > s_mountainInland && mountain > s_mountainCutoff)
    {
        return m_mountains;
    }

    if (continent > s_mountainInland && mountain > s_mountainCutoff - s_meadowBand && temperature >= s_snowTemperature)
    {
        return m_iceSpike.GetNoise(x * s_iceSpikeScale, z * s_iceSpikeScale) > s_bambooCutoff ? m_cherryGrove : m_meadow;
    }

    if (temperature < s_snowTemperature)
    {
        constexpr float kGlacierCutoff = -0.25f;
        float iceSpike                 = m_iceSpike.GetNoise(x * s_iceSpikeScale, z * s_iceSpikeScale);
        if (iceSpike > s_iceSpikeCutoff)
        {
            return m_iceSpikes;
        }

        if (iceSpike < kGlacierCutoff)
        {
            return m_glacier;
        }

        return m_snowyTundra;
    }

    if (biome < s_forestCutoff)
    {
        if (temperature < s_spruceCutoff)
        {
            return m_iceSpike.GetNoise(x * s_iceSpikeScale, z * s_iceSpikeScale) < s_oldGrowthCutoff ? m_oldGrowthSpruceTaiga : m_spruceForest;
        }

        if (biome < s_swampCutoff && temperature <= s_birchCutoff)
        {
            return m_swamp;
        }

        if (temperature > s_jungleTemperature)
        {
            return m_iceSpike.GetNoise(x * s_iceSpikeScale, z * s_iceSpikeScale) > s_bambooCutoff ? m_bambooJungle : m_jungle;
        }

        if (temperature > s_jungleTemperature - s_jungleEdgeBand)
        {
            return m_jungleEdge;
        }

        if (temperature > s_birchCutoff)
        {
            return m_birchForest;
        }

        return m_iceSpike.GetNoise(x * s_iceSpikeScale, z * s_iceSpikeScale) > s_bambooCutoff ? m_darkForest : m_forest;
    }

    if (biome > s_savannaCutoff && temperature > s_savannaTemperature)
    {
        return m_savanna;
    }

    return m_plains;
}

Biome *BiomeSource::GetCellLandBiome(int cellX, int cellZ) const
{
    struct CellCache
    {
        const BiomeSource *owner = nullptr;
        uint32_t seed            = 0;
        std::unordered_map<uint64_t, Biome *> cells;
    };

    // Generation threads share this source, so every thread keeps its own cache
    thread_local CellCache cache;
    if (cache.owner != this || cache.seed != m_seed || cache.cells.size() > 65536)
    {
        cache.owner = this;
        cache.seed  = m_seed;
        cache.cells.clear();
    }

    uint64_t key = ((uint64_t) (uint32_t) cellX << 32) | (uint64_t) (uint32_t) cellZ;
    auto found   = cache.cells.find(key);
    if (found != cache.cells.end())
    {
        return found->second;
    }

    int levelX = cellX * s_regionCellSize + s_regionCellSize / 2;
    int levelZ = cellZ * s_regionCellSize + s_regionCellSize / 2;
    float x    = 0.0f;
    float z    = 0.0f;
    Warp(levelX, levelZ, &x, &z);
    float continent   = GetContinent(x, z);
    float biome       = m_biome.GetNoise(x * s_biomeScale, z * s_biomeScale);
    float temperature = m_temperature.GetNoise(x * s_temperatureScale, z * s_temperatureScale);
    Biome *land       = GetLandBiome(x, z, continent, biome, temperature);
    cache.cells[key]  = land;
    return land;
}

// Noise that barely crosses a cutoff leaves patches only a couple of chunks wide, those take the biome around them
Biome *BiomeSource::FilterSmallRegion(int levelX, int levelZ, Biome *biome) const
{
    // The cell grid is warped so replaced patches don't end on straight chunk lines
    float edgeX = (float) levelX;
    float edgeZ = (float) levelZ;
    m_edgeWarp.DomainWarp(edgeX, edgeZ);
    int cellX = (int) std::floor(edgeX / (float) s_regionCellSize);
    int cellZ = (int) std::floor(edgeZ / (float) s_regionCellSize);

    const int maxCells = (s_regionRadius * 2 + 1) * (s_regionRadius * 2 + 1);
    Biome *neighbors[maxCells];
    int neighborCount = 0;
    int sameCount     = 0;
    for (int dz = -s_regionRadius; dz <= s_regionRadius; dz++)
    {
        for (int dx = -s_regionRadius; dx <= s_regionRadius; dx++)
        {
            Biome *cell = GetCellLandBiome(cellX + dx, cellZ + dz);
            if (cell == biome)
            {
                sameCount++;
            }
            else
            {
                neighbors[neighborCount++] = cell;
            }
        }
    }

    if (sameCount >= s_regionMinCells || neighborCount == 0)
    {
        return biome;
    }

    Biome *best   = biome;
    int bestCount = 0;
    for (int i = 0; i < neighborCount; i++)
    {
        int count = 0;
        for (int j = 0; j < neighborCount; j++)
        {
            if (neighbors[j] == neighbors[i])
            {
                count++;
            }
        }

        if (count > bestCount)
        {
            best      = neighbors[i];
            bestCount = count;
        }
    }

    return best;
}

float BiomeSource::GetContinentalness(int levelX, int levelZ) const
{
    float x = 0.0f;
    float z = 0.0f;
    Warp(levelX, levelZ, &x, &z);
    return GetContinent(x, z);
}

float BiomeSource::GetOceanDepth(float continentalness) const
{
    float t = std::clamp((s_oceanCutoff - continentalness) / s_shelfWidth, 0.0f, 1.0f);
    t       = t * t * (3.0f - 2.0f * t);
    return s_shoreDepth + (m_ocean->GetDepth() - s_shoreDepth) * t;
}

float BiomeSource::GetMountainFactor(int levelX, int levelZ) const
{
    constexpr float kPeakNoise = 0.48f;
    float x                    = 0.0f;
    float z                    = 0.0f;
    Warp(levelX, levelZ, &x, &z);
    float start = s_mountainCutoff - s_meadowBand;
    float t     = std::clamp((m_mountain.GetNoise(x * s_mountainScale, z * s_mountainScale) - start) / (kPeakNoise - start), 0.0f, 1.0f);
    return t * (2.0f - t);
}

// This is negative inside snowy biomes and rises the further the ground is from snow
float BiomeSource::GetSnowMargin(int levelX, int levelZ) const
{
    float x = 0.0f;
    float z = 0.0f;
    Warp(levelX, levelZ, &x, &z);
    return m_temperature.GetNoise(x * s_temperatureScale, z * s_temperatureScale) - s_snowTemperature;
}
