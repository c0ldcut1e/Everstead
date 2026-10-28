#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../block/Block.h"

class Biome
{
public:
    enum eTreeType
    {
        eTreeType_OAK,
        eTreeType_SPRUCE,
        eTreeType_BIRCH,
        eTreeType_ACACIA,
        eTreeType_SWAMP,
        eTreeType_JUNGLE,
        eTreeType_JUNGLE_EDGE,
        eTreeType_MEGA_SPRUCE,
        eTreeType_CHERRY,
        eTreeType_DARK_OAK
    };

    Biome();
    virtual ~Biome();

    static Biome *ById(uint32_t id);
    static Biome *ByName(const ResourceLocation &location);

    const ResourceLocation &GetResourceLocation() const;

    Block *GetTopBlock() const;
    Block *GetFillerBlock() const;

    float GetDepth() const;
    float GetScale() const;
    uint32_t GetGrassColor() const;
    uint32_t GetFoliageColor() const;
    uint32_t GetSkyColor() const;
    uint32_t GetFogColor() const;
    bool HasRain() const;
    bool IsSnowy() const;
    bool IsSnowyAt(int y) const;
    bool IsUnderground() const;
    bool SpawnsAnimals() const;
    bool SpawnsChickens() const;
    bool SpawnsJungleMobs() const;

    bool IsDecorated() const;
    float GetTreeDensity() const;
    eTreeType GetTreeType() const;
    eTreeType GetAltTreeType() const;
    int GetAltTreeChance() const;
    int GetGrassPatches() const;
    bool HasFerns() const;
    const std::vector<std::string> &GetDoublePlants() const;
    int GetDoublePlantMin() const;
    int GetDoublePlantMax() const;
    const std::vector<std::string> &GetFlowers() const;
    int GetFlowerPatches() const;
    int GetFlowerPatchChance() const;
    int GetFlowerAttempts() const;
    const std::vector<std::string> &GetNoiseFlowers() const;
    int GetNoiseFlowerAttempts() const;
    bool HasDesertPlants() const;
    int GetSugarCaneAttempts() const;
    int GetLilyPads() const;
    bool HasMelons() const;
    int GetBambooAttempts() const;
    int GetVineAttempts() const;
    bool HasDryBasins() const;
    bool HasHotCaves() const;
    bool HasIceSpikes() const;
    bool HasEmeralds() const;
    bool HasContinentalDepth() const;
    float GetMountainDepth() const;
    bool HasIceCaves() const;
    bool HasOvergrownCaves() const;
    bool HasPodzolSurface() const;
    bool HasForestRocks() const;

protected:
    std::string m_topBlock;
    std::string m_fillerBlock;
    float m_depth;
    float m_scale;
    uint32_t m_grassColor;
    uint32_t m_foliageColor;
    uint32_t m_skyColor;
    uint32_t m_fogColor;
    bool m_hasRain;
    bool m_snowy;
    int m_snowLine;
    bool m_underground;
    bool m_spawnsAnimals;
    bool m_spawnsChickens;
    bool m_spawnsJungleMobs;

    bool m_decorated;
    float m_treeDensity;
    eTreeType m_treeType;
    eTreeType m_altTreeType;
    int m_altTreeChance;
    int m_grassPatches;
    bool m_ferns;
    std::vector<std::string> m_doublePlants;
    int m_doublePlantMin;
    int m_doublePlantMax;
    std::vector<std::string> m_flowers;
    int m_flowerPatches;
    int m_flowerPatchChance;
    int m_flowerAttempts;
    std::vector<std::string> m_noiseFlowers;
    int m_noiseFlowerAttempts;
    bool m_desertPlants;
    int m_sugarCaneAttempts;
    int m_lilyPads;
    bool m_melons;
    int m_bambooAttempts;
    int m_vineAttempts;
    bool m_dryBasins;
    bool m_hotCaves;
    bool m_iceSpikes;
    bool m_emeralds;
    bool m_continentalDepth;
    float m_mountainDepth;
    bool m_iceCaves;
    bool m_overgrownCaves;
    bool m_podzolSurface;
    bool m_forestRocks;

private:
};
