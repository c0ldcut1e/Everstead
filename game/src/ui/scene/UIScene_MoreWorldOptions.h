#pragma once

#include <cstdint>
#include <string>

#include "../../world/GameRules.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_MoreWorldOptions : public UIScene
{
public:
    UIScene_MoreWorldOptions();
    ~UIScene_MoreWorldOptions() override;

    void OnGameplayAction(eGAMEPLAY_ACTION action) override;
    static std::wstring FormatMaxPlayersText(int maxPlayers);
    static void AddGameRuleCheckboxes(UIControl_MultiList *panel, const GameRules &gameRules);
    static void UpdateGameRule(GameRules *gameRules, uint32_t id, bool checked);

    struct GameRuleOption
    {
        eGAME_RULE rule;
        const char *langKey;
    };

    static const int s_gameRuleOptionCount  = 5;
    static const uint32_t s_gameRuleFirstId = 10;
    static const GameRuleOption s_gameRuleOptions[s_gameRuleOptionCount];

private:
    static const float s_moreOptionsPanelWidth;
    static const float s_moreOptionsPanelPaddingX;
    static const float s_moreOptionsPanelPaddingY;
    static const float s_moreOptionsCheckboxHeight;
    static const float s_moreOptionsSliderHeight;
    static const float s_moreOptionsRowSpacing;
    static const float s_moreOptionsMinPlayers;
    static const float s_moreOptionsMaxPlayers;
    static const uint32_t s_moreOptionsOnlineId     = 1;
    static const uint32_t s_moreOptionsFriendsId    = 2;
    static const uint32_t s_moreOptionsMaxPlayersId = 3;
    static void HandleCheckboxUpdated(void *context, uint32_t id, bool checked);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);
    void UpdateFriendsCheckboxEnabled();

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_panel;
    UIControl_Checkbox *m_friendsCheckbox;
};
