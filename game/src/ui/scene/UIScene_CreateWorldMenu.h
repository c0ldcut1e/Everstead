#pragma once

#include <cstdint>
#include <string>

#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_CreateWorldMenu : public UIScene
{
public:
    UIScene_CreateWorldMenu();
    ~UIScene_CreateWorldMenu() override;

    void OnGameplayAction(eGAMEPLAY_ACTION action) override;
    static std::wstring FormatDifficultyText(int difficulty);
    static std::wstring FormatGameTypeText(int gameType);

private:
    static const float s_worldCreationPanelWidth;
    static const float s_worldCreationPanelPaddingX;
    static const float s_worldCreationPanelPaddingY;
    static const float s_worldCreationLabelHeight;
    static const float s_worldCreationTextInputHeight;
    static const float s_worldCreationButtonHeight;
    static const float s_worldCreationSliderHeight;
    static const float s_worldCreationRowSpacing;
    static const int s_worldCreationDefaultDifficulty  = 2;
    static const uint32_t s_worldCreationNameLabelId   = 1;
    static const uint32_t s_worldCreationNameInputId   = 2;
    static const uint32_t s_worldCreationDifficultyId  = 3;
    static const uint32_t s_worldCreationMoreOptionsId = 4;
    static const uint32_t s_worldCreationCreateId      = 5;
    static const uint32_t s_worldCreationGameTypeId    = 7;
    static const uint32_t s_worldCreationSeedLabelId   = 8;
    static const uint32_t s_worldCreationSeedInputId   = 9;
    static const uint32_t s_worldCreationSeedHintId    = 10;
    static const uint32_t s_worldCreationWorldGenId    = 11;
    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);
    static std::wstring DifficultyName(int difficulty);

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_panel;
};
