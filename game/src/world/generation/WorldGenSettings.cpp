#include "WorldGenSettings.h"

#include <algorithm>
#include <memory>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../utils/StringUtils.h"
#include "../biome/Biome.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"

WorldGenSettings::WorldGenSettings()
{
    worldType       = eWorldType_Default;
    biome           = "plains";
    flatLayers      = {{"bedrock", 1}, {"dirt", 2}, {"grass_block", 1}};
    coordinateScale = 684.412;
    heightScale     = 684.412;
    depthNoiseScale = 200.0;
    noiseCaves      = true;
    caves           = true;
    ravines         = true;
    caveBiomes      = true;
    lakes           = true;
    lavaLakes       = true;
    stoneVariants   = true;
    ores            = true;
    hotDepths       = true;
    springs         = true;
    trees           = true;
    vegetation      = true;
    desertPlants    = true;
    iceSpikes       = true;
    snow            = true;
    treeDensity     = 1.0f;
    lakeChance      = 4;
    lavaLakeChance  = 8;
    oreConfigs      = {{"coal_ore", 20, 17, 0, 127, false}, {"iron_ore", 90, 9, 113, 417, true},  {"iron_ore", 10, 9, 0, 64, true},
                       {"iron_ore", 10, 4, 0, 72, false},   {"gold_ore", 2, 9, 0, 31, false},     {"diamond_ore", 1, 8, 0, 15, false},
                       {"lapis_ore", 1, 7, 0, 32, true},    {"redstone_ore", 8, 7, 0, 15, false}, {"gravel", 8, 33, 0, 255, false}};
}

void WorldGenSettings::Load(CompoundTag *tag)
{
    coordinateScale = tag->GetDouble("CoordinateScale");
    heightScale     = tag->GetDouble("HeightScale");
    depthNoiseScale = tag->GetDouble("DepthNoiseScale");
    noiseCaves      = tag->GetBoolean("NoiseCaves");
    caves           = tag->GetBoolean("Caves");
    ravines         = tag->GetBoolean("Ravines");
    caveBiomes      = tag->GetBoolean("CaveBiomes");
    lakes           = tag->GetBoolean("Lakes");
    lavaLakes       = tag->GetBoolean("LavaLakes");
    stoneVariants   = tag->GetBoolean("StoneVariants");
    ores            = tag->GetBoolean("Ores");
    hotDepths       = tag->GetBoolean("HotDepths");
    springs         = tag->GetBoolean("Springs");
    trees           = tag->GetBoolean("Trees");
    vegetation      = tag->GetBoolean("Vegetation");
    desertPlants    = tag->GetBoolean("DesertPlants");
    iceSpikes       = tag->GetBoolean("IceSpikes");
    snow            = tag->GetBoolean("Snow");
    treeDensity     = tag->GetFloat("TreeDensity");
    lakeChance      = tag->GetInt("LakeChance");
    lavaLakeChance  = tag->GetInt("LavaLakeChance");

    if (tag->Contains("WorldType"))
    {
        worldType = (eWORLD_TYPE) tag->GetInt("WorldType");
        biome     = tag->GetString("Biome");
        flatLayers.clear();
        ListTag *layers = tag->GetList("FlatLayers");
        for (size_t i = 0; i < layers->Size(); i++)
        {
            CompoundTag *layer = (CompoundTag *) layers->Get(i);
            flatLayers.push_back({layer->GetString("Block"), layer->GetInt("Height")});
        }
    }

    // Older worlds stored ore heights in a different layout, so they keep the default ore table
    if (!tag->Contains("OreVeins"))
    {
        return;
    }

    oreConfigs.clear();
    ListTag *ores = tag->GetList("OreVeins");
    for (size_t i = 0; i < ores->Size(); i++)
    {
        CompoundTag *ore = (CompoundTag *) ores->Get(i);
        oreConfigs.push_back({ore->GetString("Block"), ore->GetInt("VeinCount"), ore->GetInt("VeinSize"), ore->GetInt("MinY"), ore->GetInt("MaxY"),
                              ore->GetBoolean("Triangular")});
    }

    bool hasRedstone = false;
    for (size_t i = 0; i < oreConfigs.size(); i++)
    {
        hasRedstone |= oreConfigs[i].block == "redstone_ore";
    }

    // Worlds saved before redstone existed have no vein for it, so we add the default one
    if (!hasRedstone)
    {
        oreConfigs.push_back({"redstone_ore", 8, 7, 0, 15, false});
    }
}

void WorldGenSettings::Save(CompoundTag *tag) const
{
    tag->PutDouble("CoordinateScale", coordinateScale);
    tag->PutDouble("HeightScale", heightScale);
    tag->PutDouble("DepthNoiseScale", depthNoiseScale);
    tag->PutBoolean("NoiseCaves", noiseCaves);
    tag->PutBoolean("Caves", caves);
    tag->PutBoolean("Ravines", ravines);
    tag->PutBoolean("CaveBiomes", caveBiomes);
    tag->PutBoolean("Lakes", lakes);
    tag->PutBoolean("LavaLakes", lavaLakes);
    tag->PutBoolean("StoneVariants", stoneVariants);
    tag->PutBoolean("Ores", ores);
    tag->PutBoolean("HotDepths", hotDepths);
    tag->PutBoolean("Springs", springs);
    tag->PutBoolean("Trees", trees);
    tag->PutBoolean("Vegetation", vegetation);
    tag->PutBoolean("DesertPlants", desertPlants);
    tag->PutBoolean("IceSpikes", iceSpikes);
    tag->PutBoolean("Snow", snow);
    tag->PutFloat("TreeDensity", treeDensity);
    tag->PutInt("LakeChance", lakeChance);
    tag->PutInt("LavaLakeChance", lavaLakeChance);
    tag->PutInt("WorldType", worldType);
    tag->PutString("Biome", biome);

    std::unique_ptr<ListTag> layers(new ListTag());
    for (size_t i = 0; i < flatLayers.size(); i++)
    {
        std::unique_ptr<CompoundTag> layer(new CompoundTag());
        layer->PutString("Block", flatLayers[i].block);
        layer->PutInt("Height", flatLayers[i].height);
        layers->Add(std::move(layer));
    }

    tag->Put("FlatLayers", std::move(layers));

    std::unique_ptr<ListTag> ores(new ListTag());
    for (size_t i = 0; i < oreConfigs.size(); i++)
    {
        std::unique_ptr<CompoundTag> ore(new CompoundTag());
        ore->PutString("Block", oreConfigs[i].block);
        ore->PutInt("VeinCount", oreConfigs[i].veinCount);
        ore->PutInt("VeinSize", oreConfigs[i].veinSize);
        ore->PutInt("MinY", oreConfigs[i].minY);
        ore->PutInt("MaxY", oreConfigs[i].maxY);
        ore->PutBoolean("Triangular", oreConfigs[i].triangular);
        ores->Add(std::move(ore));
    }

    tag->Put("OreVeins", std::move(ores));
}

std::string WorldGenSettings::GetLayerCode() const
{
    std::string code;
    for (size_t i = 0; i < flatLayers.size(); i++)
    {
        if (i > 0)
        {
            code += ",";
        }

        if (flatLayers[i].height > 1)
        {
            code += std::to_string(flatLayers[i].height) + "*";
        }

        code += flatLayers[i].block;
    }

    return code;
}

bool WorldGenSettings::SetLayerCode(const std::string &code)
{
    std::vector<FlatLayer> layers;
    int total                      = 0;
    std::vector<std::string> parts = StringUtils::Split(code, ',');
    for (size_t i = 0; i < parts.size(); i++)
    {
        std::string part = StringUtils::Trim(parts[i]);
        int height       = 1;
        size_t star      = part.find('*');
        if (star != std::string::npos)
        {
            height = atoi(part.substr(0, star).c_str());
            part   = StringUtils::Trim(part.substr(star + 1));
        }

        ResourceLocation location;
        if (height <= 0 || !ResourceLocation::TryParse(part, &location) || !Block::ByName(location))
        {
            return false;
        }

        total += height;
        if (total > Chunk::s_sizeY)
        {
            return false;
        }

        layers.push_back({location.GetPath(), height});
    }

    if (layers.empty())
    {
        return false;
    }

    flatLayers = layers;
    return true;
}
