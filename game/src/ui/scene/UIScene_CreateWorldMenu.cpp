#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_CreateWorldMenu.h"

#include <cmath>
#include <string>

#include "../../core/Everstead.h"
#include "../../entity/player/GameType.h"
#include "../../utils/StringUtils.h"
#include "../Screen.h"

const float UIScene_CreateWorldMenu::s_worldCreationPanelWidth      = 430.0f;
const float UIScene_CreateWorldMenu::s_worldCreationPanelPaddingX   = 18.0f;
const float UIScene_CreateWorldMenu::s_worldCreationPanelPaddingY   = 16.0f;
const float UIScene_CreateWorldMenu::s_worldCreationLabelHeight     = 34.0f;
const float UIScene_CreateWorldMenu::s_worldCreationTextInputHeight = 46.0f;
const float UIScene_CreateWorldMenu::s_worldCreationButtonHeight    = 46.0f;
const float UIScene_CreateWorldMenu::s_worldCreationSliderHeight    = 46.0f;
const float UIScene_CreateWorldMenu::s_worldCreationRowSpacing      = 8.0f;

UIScene_CreateWorldMenu::UIScene_CreateWorldMenu() : UIScene(eUIScene_CreateWorldMenu)
{
    float contentWidth = s_worldCreationPanelWidth - s_worldCreationPanelPaddingX * 2.0f;

    m_groupRoot = new UIGroup("Group_CreateWorldMenu");
    m_panel     = new UIControl_MultiList(0.0f, 0.0f, s_worldCreationPanelWidth, 0.0f);
    m_panel->SetContentPadding(s_worldCreationPanelPaddingX, s_worldCreationPanelPaddingY);
    m_panel->SetLabelRowHeight(s_worldCreationLabelHeight);
    m_panel->SetLabelRowWidth(contentWidth);
    m_panel->SetTextInputRowWidth(contentWidth);
    m_panel->SetSliderRowWidth(contentWidth);
    m_panel->SetButtonRowWidth(contentWidth);
    m_panel->SetTextInputRowHeight(s_worldCreationTextInputHeight);
    m_panel->SetButtonRowHeight(s_worldCreationButtonHeight);
    m_panel->SetSliderRowHeight(s_worldCreationSliderHeight);
    m_panel->SetRowSpacing(s_worldCreationRowSpacing);
    m_panel->SetButtonPressedCallback(HandleButtonPressed, this);
    m_panel->SetSliderUpdatedCallback(HandleSliderUpdated, this);
    m_panel->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    Everstead *everstead                = Everstead::GetInstance();
    everstead->GetNewWorldGameRules()   = GameRules();
    everstead->GetNewWorldGenSettings() = WorldGenSettings();
    m_panel->AddNewLabel(s_worldCreationNameLabelId, everstead->GetString("world.create.world_name"));
    m_panel->AddNewTextInput(s_worldCreationNameInputId);
    m_panel->SetTextInputText(s_worldCreationNameInputId, everstead->GetString("world.create.default_name"));
    m_panel->AddNewLabel(s_worldCreationSeedLabelId, everstead->GetString("world.create.seed"));
    m_panel->AddNewTextInput(s_worldCreationSeedInputId);
    m_panel->AddNewLabel(s_worldCreationSeedHintId, everstead->GetString("world.create.seed_hint"), 0xFF7A7A7A);
    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewSlider(s_worldCreationDifficultyId, everstead->GetString("world.create.difficulty"), 0.0f, 3.0f,
                          (float) s_worldCreationDefaultDifficulty, 1.0f);
    m_panel->SetSliderText(s_worldCreationDifficultyId, FormatDifficultyText(s_worldCreationDefaultDifficulty));
    m_panel->AddNewSlider(s_worldCreationGameTypeId, everstead->GetString("world.create.game_mode"), 0.0f, (float) (eGameType_Count - 1),
                          (float) eGameType_Survival, 1.0f);
    m_panel->SetSliderText(s_worldCreationGameTypeId, FormatGameTypeText(eGameType_Survival));
    m_panel->AddNewSpacer(0, 8.0f);
    m_panel->AddNewButton(s_worldCreationWorldGenId, everstead->GetString("world.gen.title"));
    m_panel->AddNewButton(s_worldCreationMoreOptionsId, everstead->GetString("world.create.more_options"));
    m_panel->AddNewButton(s_worldCreationCreateId, everstead->GetString("world.create.create_new_world"));
    m_panel->ResizeHorizontal();
    m_panel->ResizeVertical();
    m_panel->SetPosition((Screen::WIDTH - m_panel->GetWidth()) * 0.5f, (Screen::HEIGHT - m_panel->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_panel);
    AddGroup(m_groupRoot);
}

UIScene_CreateWorldMenu::~UIScene_CreateWorldMenu()
{}

void UIScene_CreateWorldMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_CreateWorldMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_CreateWorldMenu *scene = (UIScene_CreateWorldMenu *) context;
    Everstead *everstead           = Everstead::GetInstance();

    if (id == s_worldCreationWorldGenId)
    {
        everstead->GetUiController()->NavigateToScene(eUIScene_WorldGenMenu);
        return;
    }

    if (id == s_worldCreationMoreOptionsId)
    {
        everstead->GetUiController()->NavigateToScene(eUIScene_MoreWorldOptions);
        return;
    }

    if (id != s_worldCreationCreateId)
    {
        return;
    }

    std::wstring wideName = scene->m_panel->GetTextInputText(s_worldCreationNameInputId);
    std::string worldName = StringUtils::ToUtf8(wideName);
    if (worldName.empty())
    {
        worldName = StringUtils::ToUtf8(everstead->GetString("world.create.default_name"));
    }

    int difficulty = (int) round(scene->m_panel->GetSliderValue(s_worldCreationDifficultyId));
    int gameType   = (int) round(scene->m_panel->GetSliderValue(s_worldCreationGameTypeId));
    everstead->StartNewWorld(worldName, StringUtils::ToUtf8(scene->m_panel->GetTextInputText(s_worldCreationSeedInputId)), difficulty, gameType);
}

void UIScene_CreateWorldMenu::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.scroll");
    UIScene_CreateWorldMenu *scene = (UIScene_CreateWorldMenu *) context;
    if (id == s_worldCreationDifficultyId)
    {
        scene->m_panel->SetSliderText(id, FormatDifficultyText((int) round(value)));
    }
    else if (id == s_worldCreationGameTypeId)
    {
        scene->m_panel->SetSliderText(id, FormatGameTypeText((int) round(value)));
    }
}

std::wstring UIScene_CreateWorldMenu::DifficultyName(int difficulty)
{
    Everstead *everstead = Everstead::GetInstance();
    switch (difficulty)
    {
        case 0: {
            return everstead->GetString("world.difficulty.peaceful");
        }

        case 1: {
            return everstead->GetString("world.difficulty.easy");
        }

        case 3: {
            return everstead->GetString("world.difficulty.hard");
        }

        default: {
            return everstead->GetString("world.difficulty.normal");
        }
    }
}

std::wstring UIScene_CreateWorldMenu::FormatDifficultyText(int difficulty)
{
    return Everstead::GetInstance()->GetString("world.create.difficulty") + L": " + DifficultyName(difficulty);
}

std::wstring UIScene_CreateWorldMenu::FormatGameTypeText(int gameType)
{
    Everstead *everstead = Everstead::GetInstance();
    std::string name     = GameType::GetName(GameType::IsValid(gameType) ? (eGAME_TYPE) gameType : eGameType_Survival);
    return everstead->GetString("world.create.game_mode") + L": " + everstead->GetString("world.game_mode." + name);
}

#endif
