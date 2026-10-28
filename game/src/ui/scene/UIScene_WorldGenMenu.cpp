#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_WorldGenMenu.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "../../core/Everstead.h"
#include "../../utils/StringUtils.h"
#include "../../world/biome/BiomeRegistry.h"
#include "../Screen.h"

struct WorldGenFeatureOption
{
    bool WorldGenSettings::*member;
    const char *langKey;
};

struct FlatPreset
{
    const char *langKey;
    const char *biome;
    const char *layers;
    bool features;
};

static const char *const s_worldTypeKeys[] = {"world.gen.type.default", "world.gen.type.amplified", "world.gen.type.large_biomes",
                                              "world.gen.type.flat", "world.gen.type.single_biome"};

static const WorldGenFeatureOption s_featureOptions[] = {
        {&WorldGenSettings::caves, "world.gen.feature.caves"},
        {&WorldGenSettings::noiseCaves, "world.gen.feature.noise_caves"},
        {&WorldGenSettings::ravines, "world.gen.feature.ravines"},
        {&WorldGenSettings::caveBiomes, "world.gen.feature.cave_biomes"},
        {&WorldGenSettings::hotDepths, "world.gen.feature.hot_depths"},
        {&WorldGenSettings::lakes, "world.gen.feature.lakes"},
        {&WorldGenSettings::lavaLakes, "world.gen.feature.lava_lakes"},
        {&WorldGenSettings::springs, "world.gen.feature.springs"},
        {&WorldGenSettings::ores, "world.gen.feature.ores"},
        {&WorldGenSettings::stoneVariants, "world.gen.feature.stone_variants"},
        {&WorldGenSettings::trees, "world.gen.feature.trees"},
        {&WorldGenSettings::vegetation, "world.gen.feature.vegetation"},
        {&WorldGenSettings::desertPlants, "world.gen.feature.desert_plants"},
        {&WorldGenSettings::iceSpikes, "world.gen.feature.ice_spikes"},
        {&WorldGenSettings::snow, "world.gen.feature.snow"},
};

static const FlatPreset s_flatPresets[] = {
        {"world.gen.flat.classic", "plains", "bedrock,2*dirt,grass_block", false},
        {"world.gen.flat.tunnelers_dream", "mountains", "bedrock,230*stone,5*dirt,grass_block", true},
        {"world.gen.flat.water_world", "ocean", "bedrock,5*stone,5*dirt,5*sand,90*water_still", false},
        {"world.gen.flat.overworld", "plains", "bedrock,59*stone,3*dirt,grass_block", true},
        {"world.gen.flat.snowy_kingdom", "snowy_tundra", "bedrock,59*stone,3*dirt,grass_block,snow", true},
        {"world.gen.flat.bottomless_pit", "plains", "2*cobblestone,3*dirt,grass_block", false},
        {"world.gen.flat.desert", "desert", "bedrock,3*stone,52*sandstone,8*sand", true},
        {"world.gen.flat.redstone_ready", "desert", "bedrock,3*stone,116*sandstone", false},
        {"world.gen.flat.the_void", "plains", "air", false},
};

static const int s_featureOptionCount = sizeof(s_featureOptions) / sizeof(s_featureOptions[0]);
static const int s_flatPresetCount    = sizeof(s_flatPresets) / sizeof(s_flatPresets[0]);

UIScene_WorldGenMenu::UIScene_WorldGenMenu() : UIScene(eUIScene_WorldGenMenu)
{
    m_groupRoot = new UIGroup("Group_WorldGenMenu");
    m_multiList = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_page      = ePage_Root;
    m_multiList->SetupControl(this);
    m_multiList->SetButtonTextures("assets/texture/ui/control/listbutton.png", "assets/texture/ui/control/listbutton_highlight.png");
    m_multiList->SetButtonRowHeight(46.0f);
    m_multiList->SetSliderRowHeight(46.0f);
    m_multiList->SetTextInputRowHeight(46.0f);
    m_multiList->SetLabelRowHeight(30.0f);
    m_multiList->SetCheckboxRowHeight(34.0f);
    m_multiList->SetRowSpacing(6.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetCheckboxUpdatedCallback(HandleCheckboxUpdated, this);
    m_multiList->SetSliderUpdatedCallback(HandleSliderUpdated, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    BiomeRegistry *registry = BiomeRegistry::GetInstance();
    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        if (!registry->ById(i)->IsUnderground())
        {
            m_biomes.push_back(registry->GetKey(i).GetPath());
        }
    }

    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);

    ShowRoot();
}

UIScene_WorldGenMenu::~UIScene_WorldGenMenu()
{}

void UIScene_WorldGenMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    if (m_page != ePage_Root)
    {
        ShowRoot();
        return;
    }

    if (CommitLayers())
    {
        Everstead::GetInstance()->GetUiController()->CloseScene();
    }
}

void UIScene_WorldGenMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead *everstead = Everstead::GetInstance();
    everstead->GetUiController()->PlayUiSound("ui.press");
    UIScene_WorldGenMenu *scene = (UIScene_WorldGenMenu *) context;
    WorldGenSettings &settings  = everstead->GetNewWorldGenSettings();

    if (scene->m_page == ePage_Root)
    {
        if (id == s_worldGenResetId)
        {
            settings = WorldGenSettings();
            scene->ShowRoot();
            return;
        }

        if (!scene->CommitLayers())
        {
            return;
        }

        if (id == s_worldGenFeaturesId)
        {
            scene->ShowFeatures();
        }
        else if (id == s_worldGenFlatPresetsId)
        {
            scene->ShowFlatPresets();
        }

        return;
    }

    if (scene->m_page == ePage_FlatPresets && id >= s_worldGenFlatFirstId && id < s_worldGenFlatFirstId + s_flatPresetCount)
    {
        const FlatPreset &preset = s_flatPresets[id - s_worldGenFlatFirstId];
        settings.worldType       = eWorldType_Flat;
        settings.biome           = preset.biome;
        settings.SetLayerCode(preset.layers);
        for (int i = 0; i < s_featureOptionCount; i++)
        {
            settings.*s_featureOptions[i].member = preset.features;
        }

        scene->ShowRoot();
        return;
    }
}

void UIScene_WorldGenMenu::HandleCheckboxUpdated(void *context, uint32_t id, bool checked)
{
    (void) context;
    Everstead *everstead = Everstead::GetInstance();
    everstead->GetUiController()->PlayUiSound("ui.press");
    if (id >= s_worldGenFeatureFirstId && id < s_worldGenFeatureFirstId + s_featureOptionCount)
    {
        everstead->GetNewWorldGenSettings().*s_featureOptions[id - s_worldGenFeatureFirstId].member = checked;
    }
}

void UIScene_WorldGenMenu::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.scroll");
    ((UIScene_WorldGenMenu *) context)->ApplySlider(id, value);
}

void UIScene_WorldGenMenu::ApplySlider(uint32_t id, float value)
{
    Everstead *everstead       = Everstead::GetInstance();
    WorldGenSettings &settings = everstead->GetNewWorldGenSettings();
    int step                   = (int) round(value);

    switch (id)
    {
        case s_worldGenTypeId: {
            settings.worldType = (eWORLD_TYPE) step;
            m_multiList->SetSliderText(id, FormatTypeText(step));
            UpdateRootVisibility();
            break;
        }

        case s_worldGenBiomeId: {
            settings.biome = m_biomes[step];
            m_multiList->SetSliderText(id, FormatBiomeText(step));
            break;
        }

        case s_worldGenTreeDensityId: {
            settings.treeDensity = value;
            m_multiList->SetSliderText(id,
                                       everstead->GetString("world.gen.tree_density") + L": " + std::to_wstring((int) round(value * 100.0f)) + L"%");
            break;
        }

        case s_worldGenLakeChanceId: {
            settings.lakeChance = step;
            m_multiList->SetSliderText(id, everstead->GetString("world.gen.lake_rarity") + L": 1/" + std::to_wstring(step));
            break;
        }

        case s_worldGenLavaLakeId: {
            settings.lavaLakeChance = step;
            m_multiList->SetSliderText(id, everstead->GetString("world.gen.lava_lake_rarity") + L": 1/" + std::to_wstring(step));
            break;
        }
    }
}

void UIScene_WorldGenMenu::BeginPage(ePAGE page)
{
    m_page = page;
    m_multiList->ClearEntries();
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->SetMaxVisibleElements(0);
}

void UIScene_WorldGenMenu::FinishPage()
{
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

void UIScene_WorldGenMenu::ShowRoot()
{
    Everstead *everstead             = Everstead::GetInstance();
    const WorldGenSettings &settings = everstead->GetNewWorldGenSettings();
    BeginPage(ePage_Root);

    int biomeIndex = (int) (std::find(m_biomes.begin(), m_biomes.end(), settings.biome) - m_biomes.begin());
    biomeIndex     = std::min(biomeIndex, (int) m_biomes.size() - 1);

    m_multiList->AddNewLabel(0, everstead->GetString("world.gen.title"));
    m_multiList->AddNewSlider(s_worldGenTypeId, L"", 0.0f, (float) eWorldType_SingleBiome, (float) settings.worldType, 1.0f);
    m_multiList->SetSliderText(s_worldGenTypeId, FormatTypeText(settings.worldType));
    m_multiList->AddNewSlider(s_worldGenBiomeId, L"", 0.0f, (float) (m_biomes.size() - 1), (float) biomeIndex, 1.0f);
    m_multiList->SetSliderText(s_worldGenBiomeId, FormatBiomeText(biomeIndex));
    m_multiList->AddNewLabel(s_worldGenLayersLabelId, everstead->GetString("world.gen.flat.layers"));
    m_multiList->AddNewTextInput(s_worldGenLayersInputId);
    m_multiList->SetTextInputText(s_worldGenLayersInputId, StringUtils::ToUtf16(settings.GetLayerCode()));
    m_multiList->AddNewLabel(s_worldGenLayersHintId, everstead->GetString("world.gen.flat.layers_invalid"), 0xFFB03030);
    m_multiList->AddNewButton(s_worldGenFlatPresetsId, everstead->GetString("world.gen.flat.presets"));
    m_multiList->AddNewButton(s_worldGenFeaturesId, everstead->GetString("world.gen.features"));
    m_multiList->AddNewButton(s_worldGenResetId, everstead->GetString("world.gen.reset"));
    m_multiList->ResizeVertical();
    m_multiList->SetItemVisible(s_worldGenLayersInputId, false);
    m_multiList->SetItemVisible(s_worldGenLayersHintId, false);
    m_multiList->ResizeHorizontal();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
    UpdateRootVisibility();
}

void UIScene_WorldGenMenu::ShowFeatures()
{
    Everstead *everstead             = Everstead::GetInstance();
    const WorldGenSettings &settings = everstead->GetNewWorldGenSettings();
    BeginPage(ePage_Features);

    m_multiList->AddNewLabel(0, everstead->GetString("world.gen.features"));
    for (int i = 0; i < s_featureOptionCount; i++)
    {
        m_multiList->AddNewCheckbox(s_worldGenFeatureFirstId + i, everstead->GetString(s_featureOptions[i].langKey),
                                    settings.*s_featureOptions[i].member);
    }

    m_multiList->AddNewSlider(s_worldGenTreeDensityId, L"", 0.0f, 4.0f, settings.treeDensity, 0.25f);
    m_multiList->AddNewSlider(s_worldGenLakeChanceId, L"", 1.0f, 64.0f, (float) settings.lakeChance, 1.0f);
    m_multiList->AddNewSlider(s_worldGenLavaLakeId, L"", 1.0f, 64.0f, (float) settings.lavaLakeChance, 1.0f);
    ApplySlider(s_worldGenTreeDensityId, settings.treeDensity);
    ApplySlider(s_worldGenLakeChanceId, (float) settings.lakeChance);
    ApplySlider(s_worldGenLavaLakeId, (float) settings.lavaLakeChance);
    m_multiList->SetMaxVisibleElements(10);
    m_multiList->SetFixedTopElements(1);
    FinishPage();
}

void UIScene_WorldGenMenu::ShowFlatPresets()
{
    Everstead *everstead = Everstead::GetInstance();
    BeginPage(ePage_FlatPresets);

    m_multiList->AddNewLabel(0, everstead->GetString("world.gen.flat.presets"));
    for (int i = 0; i < s_flatPresetCount; i++)
    {
        m_multiList->AddNewButton(s_worldGenFlatFirstId + i, everstead->GetString(s_flatPresets[i].langKey));
    }

    FinishPage();
}

void UIScene_WorldGenMenu::UpdateRootVisibility()
{
    eWORLD_TYPE type = Everstead::GetInstance()->GetNewWorldGenSettings().worldType;
    bool flat        = type == eWorldType_Flat;
    m_multiList->SetItemVisible(s_worldGenBiomeId, flat || type == eWorldType_SingleBiome);
    m_multiList->SetItemVisible(s_worldGenLayersLabelId, flat);
    m_multiList->SetItemVisible(s_worldGenLayersInputId, flat);
    m_multiList->SetItemVisible(s_worldGenFlatPresetsId, flat);
}

bool UIScene_WorldGenMenu::CommitLayers()
{
    Everstead *everstead       = Everstead::GetInstance();
    WorldGenSettings &settings = everstead->GetNewWorldGenSettings();
    if (settings.worldType != eWorldType_Flat || settings.SetLayerCode(StringUtils::ToUtf8(m_multiList->GetTextInputText(s_worldGenLayersInputId))))
    {
        return true;
    }

    m_multiList->SetItemVisible(s_worldGenLayersHintId, true);
    return false;
}

std::wstring UIScene_WorldGenMenu::FormatTypeText(int type) const
{
    Everstead *everstead = Everstead::GetInstance();
    return everstead->GetString("world.gen.type") + L": " + everstead->GetString(s_worldTypeKeys[type]);
}

std::wstring UIScene_WorldGenMenu::FormatBiomeText(int index) const
{
    Everstead *everstead = Everstead::GetInstance();
    return everstead->GetString("world.gen.biome") + L": " + everstead->GetString(ResourceLocation(m_biomes[index]).ToLanguageKey("biome"));
}

#endif
