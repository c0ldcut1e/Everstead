#pragma once

#include <cstdint>

#include "../../world/save/WorldSave.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_WorldOptionsMenu : public UIScene
{
public:
    UIScene_WorldOptionsMenu();
    ~UIScene_WorldOptionsMenu() override;

    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const float s_worldOptionsPanelWidth;
    static const float s_worldOptionsPanelPaddingX;
    static const float s_worldOptionsPanelPaddingY;
    static const float s_worldOptionsCheckboxHeight;
    static const float s_worldOptionsSliderHeight;
    static const float s_worldOptionsRowSpacing;
    static const uint32_t s_worldOptionsOnlineId     = 1;
    static const uint32_t s_worldOptionsFriendsId    = 2;
    static const uint32_t s_worldOptionsMaxPlayersId = 3;
    static void HandleCheckboxUpdated(void *context, uint32_t id, bool checked);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_panel;
    UIControl_Checkbox *m_friendsCheckbox;
    WorldInfo m_info;
};
