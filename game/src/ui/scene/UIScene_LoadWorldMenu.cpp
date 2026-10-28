#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_LoadWorldMenu.h"

#include <cmath>
#include <string>

#include "../../core/Everstead.h"
#include "../../entity/player/GameType.h"
#include "../../network/AccountService.h"
#include "../../rendering/TextureRepository.h"
#include "../../world/save/WorldSave.h"
#include "../Screen.h"
#include "UIScene_CreateWorldMenu.h"

#include <SDL2/SDL.h>

const float UIScene_LoadWorldMenu::s_worldInfoPanelWidth    = 430.0f;
const float UIScene_LoadWorldMenu::s_worldInfoPanelPaddingX = 18.0f;
const float UIScene_LoadWorldMenu::s_worldInfoPanelPaddingY = 16.0f;
const float UIScene_LoadWorldMenu::s_worldInfoButtonHeight  = 46.0f;
const float UIScene_LoadWorldMenu::s_worldInfoSliderHeight  = 46.0f;
const float UIScene_LoadWorldMenu::s_worldInfoRowSpacing    = 8.0f;
static const char *s_worldInfoIconTexture                   = "assets/texture/ui/scene/worldmenu_icon.png";

UIScene_LoadWorldMenu::UIScene_LoadWorldMenu() : UIScene(eUIScene_LoadWorldMenu)
{
    Everstead *everstead = Everstead::GetInstance();
    m_groupRoot          = new UIGroup("Group_LoadWorldMenu");
    m_panel              = new UIControl_MultiList(0.0f, 0.0f, s_worldInfoPanelWidth, 0.0f);
    m_panel->SetContentPadding(s_worldInfoPanelPaddingX, s_worldInfoPanelPaddingY);
    m_panel->SetLabelRowWidth(s_worldInfoPanelWidth - s_worldInfoPanelPaddingX * 2.0f);
    m_panel->SetButtonRowWidth(s_worldInfoPanelWidth - s_worldInfoPanelPaddingX * 2.0f);
    m_panel->SetSliderRowWidth(s_worldInfoPanelWidth - s_worldInfoPanelPaddingX * 2.0f);
    m_panel->SetSliderRowHeight(s_worldInfoSliderHeight);
    m_panel->SetButtonRowHeight(s_worldInfoButtonHeight);
    m_panel->SetRowSpacing(s_worldInfoRowSpacing);
    m_panel->SetButtonPressedCallback(HandleButtonPressed, this);
    m_panel->SetSliderUpdatedCallback(HandleSliderUpdated, this);
    m_panel->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    std::string selectedName = everstead->GetSelectedWorldName();
    m_info.name              = selectedName;
    m_info.iconPath          = "";
    m_info.seed              = everstead->GetSelectedWorldSeed();
    m_info.lastModified      = 0;
    m_info.valid             = false;
    WorldSave::ReadInfo(selectedName, &m_info);

    std::wstring wideName(selectedName.begin(), selectedName.end());
    std::wstring seedText = everstead->GetString("world.load.seed") + L": " + std::to_wstring(everstead->GetSelectedWorldSeed());

    TextureRepository::GetInstance()->Invalidate(m_info.iconPath);
    m_panel->AddNewIconLabel(s_worldInfoHeaderId, wideName, m_info.iconPath.empty() ? std::string(s_worldInfoIconTexture) : m_info.iconPath,
                             s_worldInfoIconTexture);
    m_panel->AddNewLabel(s_worldInfoSeedId, seedText, 0xFF7A7A7A);
    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewSlider(s_worldInfoDifficultyId, everstead->GetString("world.create.difficulty"), 0.0f, 3.0f, (float) m_info.difficulty, 1.0f);
    m_panel->SetSliderText(s_worldInfoDifficultyId, UIScene_CreateWorldMenu::FormatDifficultyText(m_info.difficulty));
    m_panel->AddNewSlider(s_worldInfoGameTypeId, everstead->GetString("world.create.game_mode"), 0.0f, (float) (eGameType_Count - 1),
                          (float) m_info.defaultGameType, 1.0f);
    m_panel->SetSliderText(s_worldInfoGameTypeId, UIScene_CreateWorldMenu::FormatGameTypeText(m_info.defaultGameType));
    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewButton(s_worldInfoCopySeedId, everstead->GetString("world.load.copy_seed"));
    m_panel->AddNewButton(s_worldInfoFolderId, everstead->GetString("world.load.open_folder"));
    m_panel->AddNewButton(s_worldInfoOptionsId, everstead->GetString("world.load.options"));
    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewButton(s_worldInfoLoadId, everstead->GetString("world.load.load"));
    m_panel->ResizeHorizontal();
    m_panel->ResizeVertical();
    m_panel->SetPosition((Screen::WIDTH - m_panel->GetWidth()) * 0.5f, (Screen::HEIGHT - m_panel->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_panel);
    AddGroup(m_groupRoot);
}

UIScene_LoadWorldMenu::~UIScene_LoadWorldMenu()
{}

void UIScene_LoadWorldMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_LoadWorldMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    Everstead *everstead         = Everstead::GetInstance();
    UIScene_LoadWorldMenu *scene = (UIScene_LoadWorldMenu *) context;
    if (id == s_worldInfoOptionsId)
    {
        everstead->GetUiController()->NavigateToScene(eUIScene_WorldOptionsMenu);
        return;
    }

    if (id == s_worldInfoCopySeedId)
    {
        SDL_SetClipboardText(std::to_string(scene->m_info.seed).c_str());
        scene->m_panel->SetButtonText(id, everstead->GetString("world.load.seed_copied"));
        return;
    }

    if (id == s_worldInfoFolderId)
    {
        SDL_OpenURL(("file://" + WorldSave::GetWorldDirectory(scene->m_info.name).GetPath()).c_str());
        return;
    }

    if (id != s_worldInfoLoadId)
    {
        return;
    }

    WorldSave::ReadInfo(scene->m_info.name, &scene->m_info);
    bool online = scene->m_info.onlineGame && everstead->GetAccountService()->IsLoggedIn();
    everstead->StartSavedWorld(everstead->GetSelectedWorldName(), online);
}

void UIScene_LoadWorldMenu::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.scroll");
    UIScene_LoadWorldMenu *scene = (UIScene_LoadWorldMenu *) context;
    WorldSave::ReadInfo(scene->m_info.name, &scene->m_info);
    if (id == s_worldInfoDifficultyId)
    {
        scene->m_info.difficulty = (int) round(value);
        scene->m_panel->SetSliderText(id, UIScene_CreateWorldMenu::FormatDifficultyText(scene->m_info.difficulty));
    }
    else if (id == s_worldInfoGameTypeId)
    {
        scene->m_info.defaultGameType = (int) round(value);
        scene->m_panel->SetSliderText(id, UIScene_CreateWorldMenu::FormatGameTypeText(scene->m_info.defaultGameType));
    }

    WorldSave::WriteSettings(scene->m_info);
}

#endif
