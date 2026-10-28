#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_HelpAndOptionsMenu.h"

#include "../../core/Everstead.h"
#include "../Screen.h"

UIScene_HelpAndOptionsMenu::UIScene_HelpAndOptionsMenu() : UIScene(eUIScene_HelpAndOptionsMenu)
{
    m_groupRoot      = new UIGroup("Group_HelpAndOptionsMenu");
    m_backgroundFill = new UIComponent_Fill(0x70000000);
    m_multiList      = {new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f)};
    m_multiList->SetPanelVisible(false);

    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    m_multiList->SetRowSpacing(15.0f);

    Everstead *everstead = Everstead::GetInstance();
    m_multiList->AddNewButton(s_helpOptionsChangeSkinId, everstead->GetString("menu.help.change_skin"))->SetVisible(false);
    m_multiList->AddNewButton(s_helpOptionsControlsId, everstead->GetString("menu.help.controls"));
    m_multiList->AddNewButton(s_helpOptionsSettingsId, everstead->GetString("menu.help.settings"));
    m_multiList->AddNewButton(s_helpOptionsCreditsId, everstead->GetString("menu.help.credits"))->SetVisible(false);

    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();

    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);

    m_backgroundFill->SetVisible(false);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_HelpAndOptionsMenu::~UIScene_HelpAndOptionsMenu()
{}

void UIScene_HelpAndOptionsMenu::OnEnter()
{
    Everstead *everstead = Everstead::GetInstance();
    bool inWorld         = everstead->GetLevel() != nullptr;
    m_backgroundFill->SetVisible(inWorld);
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
    m_multiList->ResetPointerInputState();
}

void UIScene_HelpAndOptionsMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    UIController *uiController = Everstead::GetInstance()->GetUiController();
    uiController->CloseScene();
}

void UIScene_HelpAndOptionsMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    (void) context;

    switch (id)
    {
        case s_helpOptionsControlsId: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_ControlsMenu);
            break;
        }

        case s_helpOptionsSettingsId: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_SettingsMenu);
            break;
        }

        default: {
            break;
        }
    }
}

#endif
