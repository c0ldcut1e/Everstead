#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_WorldOptionsMenu.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../network/AccountService.h"
#include "../Screen.h"
#include "UIScene_MoreWorldOptions.h"

const float UIScene_WorldOptionsMenu::s_worldOptionsPanelWidth     = 430.0f;
const float UIScene_WorldOptionsMenu::s_worldOptionsPanelPaddingX  = 18.0f;
const float UIScene_WorldOptionsMenu::s_worldOptionsPanelPaddingY  = 16.0f;
const float UIScene_WorldOptionsMenu::s_worldOptionsCheckboxHeight = 34.0f;
const float UIScene_WorldOptionsMenu::s_worldOptionsSliderHeight   = 46.0f;
const float UIScene_WorldOptionsMenu::s_worldOptionsRowSpacing     = 8.0f;

UIScene_WorldOptionsMenu::UIScene_WorldOptionsMenu() : UIScene(eUIScene_WorldOptionsMenu)
{
    Everstead *everstead = Everstead::GetInstance();
    float rowWidth       = s_worldOptionsPanelWidth - s_worldOptionsPanelPaddingX * 2.0f;
    m_groupRoot          = new UIGroup("Group_WorldOptionsMenu");
    m_panel              = new UIControl_MultiList(0.0f, 0.0f, s_worldOptionsPanelWidth, 0.0f);
    m_panel->SetContentPadding(s_worldOptionsPanelPaddingX, s_worldOptionsPanelPaddingY);
    m_panel->SetLabelRowWidth(rowWidth);
    m_panel->SetCheckboxRowWidth(rowWidth);
    m_panel->SetSliderRowWidth(rowWidth);
    m_panel->SetSliderRowHeight(s_worldOptionsSliderHeight);
    m_panel->SetCheckboxRowHeight(s_worldOptionsCheckboxHeight);
    m_panel->SetRowSpacing(s_worldOptionsRowSpacing);
    m_panel->SetCheckboxUpdatedCallback(HandleCheckboxUpdated, this);
    m_panel->SetSliderUpdatedCallback(HandleSliderUpdated, this);
    m_panel->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    m_info.name         = everstead->GetSelectedWorldName();
    m_info.iconPath     = "";
    m_info.seed         = everstead->GetSelectedWorldSeed();
    m_info.lastModified = 0;
    m_info.valid        = false;
    WorldSave::ReadInfo(m_info.name, &m_info);

    bool loggedIn = everstead->GetAccountService()->IsLoggedIn();
    m_panel->AddNewLabel(0, everstead->GetString("world.options.multiplayer"));
    m_panel->AddNewCheckbox(s_worldOptionsOnlineId, everstead->GetString("world.online_game"), m_info.onlineGame && loggedIn)->SetEnabled(loggedIn);
    m_friendsCheckbox = m_panel->AddNewCheckbox(s_worldOptionsFriendsId, everstead->GetString("world.create.allow_friends_of_friends"),
                                                m_info.allowFriendsOfFriends);
    m_friendsCheckbox->SetEnabled(m_info.onlineGame && loggedIn);
    m_panel->AddNewSlider(s_worldOptionsMaxPlayersId, everstead->GetString("world.create.max_players"), 2.0f, 32.0f, (float) m_info.maxPlayers, 1.0f);
    m_panel->SetSliderText(s_worldOptionsMaxPlayersId, UIScene_MoreWorldOptions::FormatMaxPlayersText(m_info.maxPlayers));
    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewLabel(0, everstead->GetString("world.options.game_rules"));
    UIScene_MoreWorldOptions::AddGameRuleCheckboxes(m_panel, m_info.gameRules);
    m_panel->ResizeHorizontal();
    m_panel->ResizeVertical();
    m_panel->SetPosition((Screen::WIDTH - m_panel->GetWidth()) * 0.5f, (Screen::HEIGHT - m_panel->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_panel);
    AddGroup(m_groupRoot);
}

UIScene_WorldOptionsMenu::~UIScene_WorldOptionsMenu()
{}

void UIScene_WorldOptionsMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_WorldOptionsMenu::HandleCheckboxUpdated(void *context, uint32_t id, bool checked)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_WorldOptionsMenu *scene = (UIScene_WorldOptionsMenu *) context;
    if (id == s_worldOptionsOnlineId)
    {
        scene->m_info.onlineGame = checked;
        scene->m_friendsCheckbox->SetEnabled(checked);
    }
    else if (id == s_worldOptionsFriendsId)
    {
        scene->m_info.allowFriendsOfFriends = checked;
    }
    else
    {
        UIScene_MoreWorldOptions::UpdateGameRule(&scene->m_info.gameRules, id, checked);
    }

    WorldSave::WriteSettings(scene->m_info);
}

void UIScene_WorldOptionsMenu::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.scroll");
    UIScene_WorldOptionsMenu *scene = (UIScene_WorldOptionsMenu *) context;
    if (id != s_worldOptionsMaxPlayersId)
    {
        return;
    }

    scene->m_info.maxPlayers = (int) round(value);
    scene->m_panel->SetSliderText(id, UIScene_MoreWorldOptions::FormatMaxPlayersText(scene->m_info.maxPlayers));
    WorldSave::WriteSettings(scene->m_info);
}

#endif
