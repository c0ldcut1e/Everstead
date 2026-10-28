#include "Biome.h"
#include "BiomeRegistry.h"

Biome::Biome()
{
    m_topBlock         = "grass_block";
    m_fillerBlock      = "dirt";
    m_depth            = 0.1f;
    m_scale            = 0.3f;
    m_grassColor       = 0x8EB971;
    m_foliageColor     = 0x71A74D;
    m_skyColor         = 0x7FA7FF;
    m_fogColor         = 0xBFD4FF;
    m_hasRain          = true;
    m_snowy            = false;
    m_snowLine         = -1;
    m_underground      = false;
    m_spawnsAnimals    = true;
    m_spawnsChickens   = true;
    m_spawnsJungleMobs = false;

    m_decorated           = false;
    m_treeDensity         = 0.0f;
    m_treeType            = eTreeType_OAK;
    m_altTreeType         = eTreeType_OAK;
    m_altTreeChance       = 0;
    m_grassPatches        = 1;
    m_ferns               = false;
    m_doublePlantMin      = 0;
    m_doublePlantMax      = 0;
    m_flowerPatches       = 0;
    m_flowerPatchChance   = 1;
    m_flowerAttempts      = 0;
    m_noiseFlowerAttempts = 0;
    m_desertPlants        = false;
    m_sugarCaneAttempts   = 0;
    m_lilyPads            = 0;
    m_melons              = false;
    m_bambooAttempts      = 0;
    m_vineAttempts        = 0;
    m_dryBasins           = false;
    m_hotCaves            = false;
    m_iceSpikes           = false;
    m_emeralds            = false;
    m_continentalDepth    = false;
    m_mountainDepth       = 0.0f;
    m_iceCaves            = true;
    m_overgrownCaves      = false;
    m_podzolSurface       = false;
    m_forestRocks         = false;
}

Biome::~Biome()
{}

Biome *Biome::ById(uint32_t id)
{
    return BiomeRegistry::GetInstance()->ById(id);
}

Biome *Biome::ByName(const ResourceLocation &location)
{
    return BiomeRegistry::GetInstance()->Get(location);
}

const ResourceLocation &Biome::GetResourceLocation() const
{
    BiomeRegistry *registry = BiomeRegistry::GetInstance();
    return registry->GetKey(registry->IdOf((Biome *) this));
}

Block *Biome::GetTopBlock() const
{
    return Block::ByName(m_topBlock);
}

Block *Biome::GetFillerBlock() const
{
    return Block::ByName(m_fillerBlock);
}

float Biome::GetDepth() const
{
    return m_depth;
}

float Biome::GetScale() const
{
    return m_scale;
}

uint32_t Biome::GetGrassColor() const
{
    return m_grassColor;
}

uint32_t Biome::GetFoliageColor() const
{
    return m_foliageColor;
}

uint32_t Biome::GetSkyColor() const
{
    return m_skyColor;
}

uint32_t Biome::GetFogColor() const
{
    return m_fogColor;
}

bool Biome::HasRain() const
{
    return m_hasRain;
}

bool Biome::IsSnowy() const
{
    return m_snowy;
}

// This checks for snow at a height, so tall biomes can have snowy peaks
bool Biome::IsSnowyAt(int y) const
{
    return m_snowy || (m_snowLine >= 0 && y >= m_snowLine);
}

bool Biome::IsUnderground() const
{
    return m_underground;
}

bool Biome::SpawnsAnimals() const
{
    return m_spawnsAnimals;
}

bool Biome::SpawnsChickens() const
{
    return m_spawnsChickens;
}

bool Biome::SpawnsJungleMobs() const
{
    return m_spawnsJungleMobs;
}

bool Biome::IsDecorated() const
{
    return m_decorated;
}

float Biome::GetTreeDensity() const
{
    return m_treeDensity;
}

Biome::eTreeType Biome::GetTreeType() const
{
    return m_treeType;
}

Biome::eTreeType Biome::GetAltTreeType() const
{
    return m_altTreeType;
}

int Biome::GetAltTreeChance() const
{
    return m_altTreeChance;
}

int Biome::GetGrassPatches() const
{
    return m_grassPatches;
}

bool Biome::HasFerns() const
{
    return m_ferns;
}

const std::vector<std::string> &Biome::GetDoublePlants() const
{
    return m_doublePlants;
}

int Biome::GetDoublePlantMin() const
{
    return m_doublePlantMin;
}

int Biome::GetDoublePlantMax() const
{
    return m_doublePlantMax;
}

const std::vector<std::string> &Biome::GetFlowers() const
{
    return m_flowers;
}

int Biome::GetFlowerPatches() const
{
    return m_flowerPatches;
}

int Biome::GetFlowerPatchChance() const
{
    return m_flowerPatchChance;
}

int Biome::GetFlowerAttempts() const
{
    return m_flowerAttempts;
}

const std::vector<std::string> &Biome::GetNoiseFlowers() const
{
    return m_noiseFlowers;
}

int Biome::GetNoiseFlowerAttempts() const
{
    return m_noiseFlowerAttempts;
}

bool Biome::HasDesertPlants() const
{
    return m_desertPlants;
}

int Biome::GetSugarCaneAttempts() const
{
    return m_sugarCaneAttempts;
}

int Biome::GetLilyPads() const
{
    return m_lilyPads;
}

bool Biome::HasMelons() const
{
    return m_melons;
}

int Biome::GetBambooAttempts() const
{
    return m_bambooAttempts;
}

int Biome::GetVineAttempts() const
{
    return m_vineAttempts;
}

bool Biome::HasDryBasins() const
{
    return m_dryBasins;
}

bool Biome::HasHotCaves() const
{
    return m_hotCaves;
}

bool Biome::HasIceSpikes() const
{
    return m_iceSpikes;
}

bool Biome::HasEmeralds() const
{
    return m_emeralds;
}

bool Biome::HasContinentalDepth() const
{
    return m_continentalDepth;
}

float Biome::GetMountainDepth() const
{
    return m_mountainDepth;
}

bool Biome::HasIceCaves() const
{
    return m_iceCaves;
}

bool Biome::HasOvergrownCaves() const
{
    return m_overgrownCaves;
}

bool Biome::HasPodzolSurface() const
{
    return m_podzolSurface;
}

bool Biome::HasForestRocks() const
{
    return m_forestRocks;
}
