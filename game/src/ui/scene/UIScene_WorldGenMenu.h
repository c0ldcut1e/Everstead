#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_WorldGenMenu : public UIScene
{
public:
    enum ePAGE : uint8_t
    {
        ePage_Root        = 0,
        ePage_Features    = 1,
        ePage_FlatPresets = 2,
    };

    UIScene_WorldGenMenu();
    ~UIScene_WorldGenMenu() override;

    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_worldGenTypeId         = 1;
    static const uint32_t s_worldGenBiomeId        = 2;
    static const uint32_t s_worldGenLayersLabelId  = 3;
    static const uint32_t s_worldGenLayersInputId  = 4;
    static const uint32_t s_worldGenLayersHintId   = 5;
    static const uint32_t s_worldGenFlatPresetsId  = 6;
    static const uint32_t s_worldGenFeaturesId     = 7;
    static const uint32_t s_worldGenResetId        = 10;
    static const uint32_t s_worldGenTreeDensityId  = 30;
    static const uint32_t s_worldGenLakeChanceId   = 31;
    static const uint32_t s_worldGenLavaLakeId     = 32;
    static const uint32_t s_worldGenFeatureFirstId = 100;
    static const uint32_t s_worldGenFlatFirstId    = 200;

    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleCheckboxUpdated(void *context, uint32_t id, bool checked);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);

    void BeginPage(ePAGE page);
    void FinishPage();

    void ShowRoot();
    void ShowFeatures();
    void ShowFlatPresets();

    void ApplySlider(uint32_t id, float value);
    void UpdateRootVisibility();
    bool CommitLayers();

    std::wstring FormatTypeText(int type) const;
    std::wstring FormatBiomeText(int index) const;

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_multiList;
    ePAGE m_page;
    std::vector<std::string> m_biomes;
};
