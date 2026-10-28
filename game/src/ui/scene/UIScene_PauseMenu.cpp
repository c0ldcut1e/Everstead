#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_PauseMenu.h"

#include "../../core/Everstead.h"
#include "../../network/AccountService.h"
#include "../../network/MultiplayerSession.h"
#include "../Screen.h"

#define PAUSE_MENU_RESUME_ID 1
#define PAUSE_MENU_SAVE_ID   2
#define PAUSE_MENU_HELP_ID   3
#define PAUSE_MENU_INVITE_ID 4
#define PAUSE_MENU_EXIT_ID   5

UIScene_PauseMenu::UIScene_PauseMenu() : UIScene(eUIScene_PauseMenu)
{
    m_groupRoot      = new UIGroup("Group_PauseMenu");
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
    m_multiList->AddNewButton(PAUSE_MENU_RESUME_ID, everstead->GetString("menu.pause.resume_game"));
    m_saveButton = m_multiList->AddNewButton(PAUSE_MENU_SAVE_ID, everstead->GetString("menu.pause.save_world"));
    m_multiList->AddNewButton(PAUSE_MENU_HELP_ID, everstead->GetString("menu.main.help_and_options"));
    m_multiList->AddNewButton(PAUSE_MENU_INVITE_ID, everstead->GetString("invitation.invites"));
    m_multiList->AddNewButton(PAUSE_MENU_EXIT_ID, everstead->GetString("menu.main.exit_game"));

    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_PauseMenu::~UIScene_PauseMenu()
{}

void UIScene_PauseMenu::OnEnter()
{
    m_saveButton->SetEnabled(Everstead::GetInstance()->CanSaveWorld());
    m_multiList->SetItemVisible(PAUSE_MENU_SAVE_ID, Everstead::GetInstance()->GetMultiplayerSession()->IsHostingSession());
    m_multiList->SetItemVisible(PAUSE_MENU_INVITE_ID, Everstead::GetInstance()->GetAccountService()->IsLoggedIn());
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);

    m_multiList->SetVisible(true);
    m_multiList->ResetPointerInputState();
}

void UIScene_PauseMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->ClosePauseMenu();
}

void UIScene_PauseMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    (void) context;

    switch (id)
    {
        case PAUSE_MENU_RESUME_ID: {
            Everstead::GetInstance()->ClosePauseMenu();
            break;
        }

        case PAUSE_MENU_SAVE_ID: {
            Everstead::GetInstance()->SaveWorld();
            break;
        }

        case PAUSE_MENU_HELP_ID: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_HelpAndOptionsMenu);
            break;
        }

        case PAUSE_MENU_INVITE_ID: {
            Everstead::GetInstance()->GetUiController()->OpenScene(eUIScene_InvitationMenu);
            break;
        }

        case PAUSE_MENU_EXIT_ID: {
            Everstead *everstead = Everstead::GetInstance();
            if (!everstead->GetMultiplayerSession()->IsHostingSession())
            {
                everstead->RequestLeaveWorld(true);
                break;
            }

            UIScene_ModalDialog::Dialog dialog;
            dialog.title    = everstead->GetString("menu.main.exit_game");
            dialog.message  = everstead->GetString("menu.pause.exit_message");
            dialog.callback = HandleExitButtonPressed;

            UIScene_ModalDialog::Button saveButton;
            saveButton.id    = eExitButton_Save;
            saveButton.label = everstead->GetString("menu.pause.exit_save");
            dialog.buttons.push_back(saveButton);

            UIScene_ModalDialog::Button noSaveButton;
            noSaveButton.id    = eExitButton_NoSave;
            noSaveButton.label = everstead->GetString("menu.pause.exit_no_save");
            dialog.buttons.push_back(noSaveButton);

            UIScene_ModalDialog::Button cancelButton;
            cancelButton.id    = eExitButton_Cancel;
            cancelButton.label = everstead->GetString("button.cancel");
            dialog.buttons.push_back(cancelButton);

            everstead->GetUiController()->ShowModal(dialog);
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_PauseMenu::HandleExitButtonPressed(void *context, uint32_t id)
{
    (void) context;
    if (id == eExitButton_Cancel)
    {
        return;
    }

    Everstead::GetInstance()->RequestLeaveWorld(id == eExitButton_Save);
}

#endif
