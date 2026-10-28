#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_MainMenu.h"

#include "../../core/Everstead.h"
#include "../../core/EversteadApp.h"
#include "../../core/GameInfo.h"
#include "../../network/AccountService.h"
#include "../Screen.h"

UIScene_MainMenu::UIScene_MainMenu() : UIScene(eUIScene_MainMenu)
{
    m_groupRoot    = new UIGroup("Group_MainMenu");
    m_multiList    = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_lastLoggedIn = false;

    m_multiList->SetPanelVisible(false);

    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);

    m_multiList->SetRowSpacing(15.0f);

    Everstead *everstead = Everstead::GetInstance();
    m_multiList->AddNewButton(1, everstead->GetString("menu.main.play_game"));
    m_serversButton = m_multiList->AddNewButton(9, everstead->GetString("menu.main.servers"));
    m_multiList->AddNewButton(2, everstead->GetString("menu.main.minigames"))->SetVisible(false);
    m_multiList->AddNewButton(3, everstead->GetString("menu.main.achievements"))->SetVisible(false);
    m_multiList->AddNewButton(6, everstead->GetString("menu.main.help_and_options"));
    m_multiList->AddNewButton(7, everstead->GetString("menu.main.account"));
    m_multiList->AddNewButton(8, everstead->GetString("menu.main.exit_game"));

    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();

    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_MainMenu::~UIScene_MainMenu()
{}

void UIScene_MainMenu::OnEnter()
{
    RefreshButtons();
}

void UIScene_MainMenu::Tick()
{
    UIScene::Tick();

    if (Everstead::GetInstance()->GetAccountService()->IsLoggedIn() != m_lastLoggedIn)
    {
        RefreshButtons();
    }
}

void UIScene_MainMenu::RefreshButtons()
{
    m_lastLoggedIn = Everstead::GetInstance()->GetAccountService()->IsLoggedIn();
    m_serversButton->SetEnabled(m_lastLoggedIn && !kStickFight);
}

void UIScene_MainMenu::HandleButtonPressed(void *context, uint32_t id)
{
    (void) context;

    switch (id)
    {
        case 1: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_LoadCreateJoin);
            break;
        }

        case 9: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_ServersMenu);
            break;
        }

        case 6: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_HelpAndOptionsMenu);
            break;
        }

        case 7: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_NetworkMenu);
            break;
        }

        case 8: {
            EversteadApp::GetInstance()->RequestClose();
            break;
        }

        default: {
            break;
        }
    }
}

#endif
