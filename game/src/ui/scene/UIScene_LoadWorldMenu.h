#pragma once

#include <cstdint>

#include "../../world/save/WorldSave.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_LoadWorldMenu : public UIScene
{
public:
    UIScene_LoadWorldMenu();
    ~UIScene_LoadWorldMenu() override;

    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const float s_worldInfoPanelWidth;
    static const float s_worldInfoPanelPaddingX;
    static const float s_worldInfoPanelPaddingY;
    static const float s_worldInfoButtonHeight;
    static const float s_worldInfoSliderHeight;
    static const float s_worldInfoRowSpacing;
    static const uint32_t s_worldInfoHeaderId     = 1;
    static const uint32_t s_worldInfoSeedId       = 2;
    static const uint32_t s_worldInfoOptionsId    = 3;
    static const uint32_t s_worldInfoLoadId       = 4;
    static const uint32_t s_worldInfoDifficultyId = 6;
    static const uint32_t s_worldInfoGameTypeId   = 7;
    static const uint32_t s_worldInfoCopySeedId   = 8;
    static const uint32_t s_worldInfoFolderId     = 9;
    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_panel;
    WorldInfo m_info;
};
