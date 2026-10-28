#pragma once

#include <FastNoiseLite.h>
#include <cstdint>

#include "../biome/Biome.h"
#include "RiverLayer.h"
#include "WorldGenSettings.h"

class BiomeSource
{
public:
    BiomeSource(uint32_t seed, const WorldGenSettings &settings);

    Biome *GetBiome(int levelX, int levelZ) const;
    float GetContinentalness(int levelX, int levelZ) const;
    float GetOceanDepth(float continentalness) const;
    float GetMountainFactor(int levelX, int levelZ) const;
    float GetSnowMargin(int levelX, int levelZ) const;

private:
    Biome *m_ocean;
    Biome *m_plains;
    Biome *m_desert;
    Biome *m_beach;
    Biome *m_forest;
    Biome *m_spruceForest;
    Biome *m_birchForest;
    Biome *m_savanna;
    Biome *m_snowyTundra;
    Biome *m_mountains;
    Biome *m_river;
    Biome *m_frozenRiver;
    Biome *m_iceSpikes;
    Biome *m_glacier;
    Biome *m_cherryGrove;
    Biome *m_meadow;
    Biome *m_swamp;
    Biome *m_jungle;
    Biome *m_jungleEdge;
    Biome *m_bambooJungle;
    Biome *m_oldGrowthSpruceTaiga;
    Biome *m_badlands;
    Biome *m_woodedBadlands;
    Biome *m_darkForest;
    Biome *m_stonyShore;

    FastNoiseLite m_continent;
    FastNoiseLite m_island;
    FastNoiseLite m_biome;
    FastNoiseLite m_temperature;
    FastNoiseLite m_warp;
    FastNoiseLite m_edgeWarp;
    FastNoiseLite m_mountain;
    FastNoiseLite m_iceSpike;
    RiverLayer m_riverLayer;

    uint32_t m_seed;
    float m_zoom;
    Biome *m_fixedBiome;

    void Warp(int levelX, int levelZ, float *outX, float *outZ) const;
    float GetContinent(float x, float z) const;
    Biome *GetLandBiome(float x, float z, float continent, float biome, float temperature) const;
    Biome *GetCellLandBiome(int cellX, int cellZ) const;
    Biome *FilterSmallRegion(int levelX, int levelZ, Biome *biome) const;

    static const float s_continentScale;
    static const float s_biomeScale;
    static const float s_oceanCutoff;
    static const float s_desertCutoff;
    static const float s_desertTemperature;
    static const float s_forestCutoff;
    static const float s_temperatureScale;
    static const float s_spruceCutoff;
    static const float s_birchCutoff;
    static const float s_swampCutoff;
    static const float s_savannaCutoff;
    static const float s_savannaTemperature;
    static const float s_snowTemperature;
    static const float s_beachWidth;
    static const float s_shelfWidth;
    static const float s_shoreDepth;
    static const float s_warpFrequency;
    static const float s_warpAmplitude;
    static const float s_edgeWarpFrequency;
    static const float s_edgeWarpAmplitude;
    static const float s_mountainScale;
    static const float s_mountainCutoff;
    static const float s_mountainInland;
    static const float s_iceSpikeScale;
    static const float s_iceSpikeCutoff;
    static const float s_meadowBand;
    static const float s_jungleTemperature;
    static const float s_jungleEdgeBand;
    static const float s_bambooCutoff;
    static const float s_oldGrowthCutoff;
    static const int s_regionCellSize;
    static const int s_regionRadius;
    static const int s_regionMinCells;
};
