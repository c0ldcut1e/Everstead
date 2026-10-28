#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_MoreWorldOptions.h"

#include <cmath>
#include <string>

#include "../../core/Everstead.h"
#include "../../network/AccountService.h"
#include "../Screen.h"

const float UIScene_MoreWorldOptions::s_moreOptionsPanelWidth     = 430.0f;
const float UIScene_MoreWorldOptions::s_moreOptionsPanelPaddingX  = 18.0f;
const float UIScene_MoreWorldOptions::s_moreOptionsPanelPaddingY  = 16.0f;
const float UIScene_MoreWorldOptions::s_moreOptionsCheckboxHeight = 34.0f;
const float UIScene_MoreWorldOptions::s_moreOptionsSliderHeight   = 46.0f;
const float UIScene_MoreWorldOptions::s_moreOptionsRowSpacing     = 8.0f;
const float UIScene_MoreWorldOptions::s_moreOptionsMinPlayers     = 2.0f;
const float UIScene_MoreWorldOptions::s_moreOptionsMaxPlayers     = 32.0f;

const UIScene_MoreWorldOptions::GameRuleOption UIScene_MoreWorldOptions::s_gameRuleOptions[s_gameRuleOptionCount] = {
        {eGameRule_KeepInventory, "world.rules.keep_inventory"},         {eGameRule_DoDaylightCycle, "world.rules.daylight_cycle"},
        {eGameRule_DoWeatherCycle, "world.rules.weather_cycle"},         {eGameRule_DoMobSpawning, "world.rules.mob_spawning"},
        {eGameRule_DoImmediateRespawn, "world.rules.immediate_respawn"},
};

UIScene_MoreWorldOptions::UIScene_MoreWorldOptions() : UIScene(eUIScene_MoreWorldOptions)
{
    float contentWidth = s_moreOptionsPanelWidth - s_moreOptionsPanelPaddingX * 2.0f;

    m_groupRoot = new UIGroup("Group_MoreWorldOptions");
    m_panel     = new UIControl_MultiList(0.0f, 0.0f, s_moreOptionsPanelWidth, 0.0f);
    m_panel->SetContentPadding(s_moreOptionsPanelPaddingX, s_moreOptionsPanelPaddingY);
    m_panel->SetLabelRowWidth(contentWidth);
    m_panel->SetCheckboxRowWidth(contentWidth);
    m_panel->SetSliderRowWidth(contentWidth);
    m_panel->SetCheckboxRowHeight(s_moreOptionsCheckboxHeight);
    m_panel->SetSliderRowHeight(s_moreOptionsSliderHeight);
    m_panel->SetRowSpacing(s_moreOptionsRowSpacing);
    m_panel->SetCheckboxUpdatedCallback(HandleCheckboxUpdated, this);
    m_panel->SetSliderUpdatedCallback(HandleSliderUpdated, this);
    m_panel->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    Everstead *everstead = Everstead::GetInstance();
    bool loggedIn        = everstead->GetAccountService()->IsLoggedIn();
    m_panel->AddNewLabel(0, everstead->GetString("world.options.multiplayer"));
    m_panel->AddNewCheckbox(s_moreOptionsOnlineId, everstead->GetString("world.online_game"), everstead->options.onlineGame && loggedIn)
            ->SetEnabled(loggedIn);
    m_friendsCheckbox = m_panel->AddNewCheckbox(s_moreOptionsFriendsId, everstead->GetString("world.create.allow_friends_of_friends"),
                                                everstead->options.allowFriendsOfFriends);
    m_panel->AddNewSlider(s_moreOptionsMaxPlayersId, everstead->GetString("world.create.max_players"), s_moreOptionsMinPlayers,
                          s_moreOptionsMaxPlayers, (float) everstead->options.maxPlayers, 1.0f);
    m_panel->SetSliderText(s_moreOptionsMaxPlayersId, FormatMaxPlayersText(everstead->options.maxPlayers));

    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewLabel(0, everstead->GetString("world.options.game_rules"));
    AddGameRuleCheckboxes(m_panel, everstead->GetNewWorldGameRules());
    UpdateFriendsCheckboxEnabled();

    m_panel->ResizeHorizontal();
    m_panel->ResizeVertical();
    m_panel->SetPosition((Screen::WIDTH - m_panel->GetWidth()) * 0.5f, (Screen::HEIGHT - m_panel->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_panel);
    AddGroup(m_groupRoot);
}

UIScene_MoreWorldOptions::~UIScene_MoreWorldOptions()
{}

void UIScene_MoreWorldOptions::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_MoreWorldOptions::HandleCheckboxUpdated(void *context, uint32_t id, bool checked)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_MoreWorldOptions *scene = (UIScene_MoreWorldOptions *) context;
    Everstead *everstead            = Everstead::GetInstance();

    if (id == s_moreOptionsOnlineId)
    {
        everstead->options.onlineGame = checked;
        scene->UpdateFriendsCheckboxEnabled();
        everstead->SaveOptions();
        return;
    }

    if (id == s_moreOptionsFriendsId)
    {
        everstead->options.allowFriendsOfFriends = checked;
        everstead->SaveOptions();
        return;
    }

    UpdateGameRule(&everstead->GetNewWorldGameRules(), id, checked);
}

void UIScene_MoreWorldOptions::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.scroll");
    if (id != s_moreOptionsMaxPlayersId)
    {
        return;
    }

    UIScene_MoreWorldOptions *scene = (UIScene_MoreWorldOptions *) context;
    Everstead *everstead            = Everstead::GetInstance();
    everstead->options.maxPlayers   = (int) round(value);
    scene->m_panel->SetSliderText(id, FormatMaxPlayersText(everstead->options.maxPlayers));
    everstead->SaveOptions();
}

std::wstring UIScene_MoreWorldOptions::FormatMaxPlayersText(int maxPlayers)
{
    return Everstead::GetInstance()->GetString("world.create.max_players") + L": " + std::to_wstring(maxPlayers);
}

void UIScene_MoreWorldOptions::AddGameRuleCheckboxes(UIControl_MultiList *panel, const GameRules &gameRules)
{
    Everstead *everstead = Everstead::GetInstance();
    for (int i = 0; i < s_gameRuleOptionCount; i++)
    {
        panel->AddNewCheckbox(s_gameRuleFirstId + i, everstead->GetString(s_gameRuleOptions[i].langKey),
                              gameRules.GetBool(s_gameRuleOptions[i].rule));
    }
}

void UIScene_MoreWorldOptions::UpdateGameRule(GameRules *gameRules, uint32_t id, bool checked)
{
    if (id < s_gameRuleFirstId || id >= s_gameRuleFirstId + s_gameRuleOptionCount)
    {
        return;
    }

    gameRules->Set(s_gameRuleOptions[id - s_gameRuleFirstId].rule, checked ? 1 : 0);
}

void UIScene_MoreWorldOptions::UpdateFriendsCheckboxEnabled()
{
    Everstead *everstead = Everstead::GetInstance();
    bool loggedIn        = everstead->GetAccountService()->IsLoggedIn();
    m_friendsCheckbox->SetEnabled(everstead->options.onlineGame && loggedIn);
}

#endif
