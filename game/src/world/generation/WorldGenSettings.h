#pragma once

#include <cstdint>
#include <string>
#include <vector>

class CompoundTag;

enum eWORLD_TYPE : uint8_t
{
    eWorldType_Default     = 0,
    eWorldType_Amplified   = 1,
    eWorldType_LargeBiomes = 2,
    eWorldType_Flat        = 3,
    eWorldType_SingleBiome = 4,
    eWorldType_Caves       = 5,
    eWorldType_Count       = 6,
};

struct FlatLayer
{
    std::string block;
    int height;

    bool operator==(const FlatLayer &other) const = default;
};

struct OreConfig
{
    std::string block;
    int veinCount;
    int veinSize;
    int minY;
    int maxY;
    bool triangular;

    bool operator==(const OreConfig &other) const = default;
};

struct WorldGenSettings
{
    WorldGenSettings();

    void Load(CompoundTag *tag);
    void Save(CompoundTag *tag) const;
    std::string GetLayerCode() const;
    bool SetLayerCode(const std::string &code);

    bool operator==(const WorldGenSettings &other) const = default;

    eWORLD_TYPE worldType;
    std::string biome;
    std::vector<FlatLayer> flatLayers;
    double coordinateScale;
    double heightScale;
    double depthNoiseScale;
    bool noiseCaves;
    bool caves;
    bool ravines;
    bool caveBiomes;
    bool lakes;
    bool lavaLakes;
    bool stoneVariants;
    bool ores;
    bool hotDepths;
    bool springs;
    bool trees;
    bool vegetation;
    bool desertPlants;
    bool iceSpikes;
    bool snow;
    float treeDensity;
    int lakeChance;
    int lavaLakeChance;
    std::vector<OreConfig> oreConfigs;
};
