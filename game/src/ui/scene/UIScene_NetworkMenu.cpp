#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_NetworkMenu.h"

#include <cstdint>
#include <string>

#include "../../core/Everstead.h"
#include "../../core/Logger.h"
#include "../../network/AccountService.h"
#include "../../utils/StringUtils.h"
#include "../Screen.h"

UIScene_NetworkMenu::UIScene_NetworkMenu() : UIScene(eUIScene_NetworkMenu)
{
    m_groupRoot      = new UIGroup("Group_NetworkMenu");
    m_backgroundFill = new UIComponent_Fill(0x70000000);
    m_multiList      = {new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f)};
    m_statusText     = {};
    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetTextInputRowWidth(600.0f);
    m_multiList->SetTextInputRowHeight(50.0f);
    m_multiList->SetLabelRowWidth(600.0f);
    m_multiList->SetLabelRowHeight(24.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(15.0f);

    Everstead *everstead = Everstead::GetInstance();
    m_multiList->AddNewLabel(s_networkMenuStatusId, everstead->GetString("account.title"));
    m_multiList->AddNewLabel(s_networkMenuUsernameLabelId, everstead->GetString("account.username") + L":");
    m_multiList->AddNewTextInput(s_networkMenuUsernameId);
    m_multiList->AddNewLabel(s_networkMenuPasswordLabelId, everstead->GetString("account.password") + L":");
    m_multiList->AddNewTextInput(s_networkMenuPasswordId)->SetPassword(true);
    m_multiList->AddNewButton(s_networkMenuLoginId, everstead->GetString("account.login"));
    m_multiList->AddNewButton(s_networkMenuFriendsId, everstead->GetString("menu.main.friends"));
    m_multiList->AddNewButton(s_networkMenuInvitesId, everstead->GetString("invitation.invites"));
    m_multiList->AddNewButton(s_networkMenuLogoutId, everstead->GetString("account.logout"));
    m_multiList->AddNewButton(s_networkMenuBackId, everstead->GetString("button.back"));

    m_backgroundFill->SetVisible(false);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);

    RefreshView();
}

UIScene_NetworkMenu::~UIScene_NetworkMenu()
{}

void UIScene_NetworkMenu::OnEnter()
{
    AccountService *accountService = Everstead::GetInstance()->GetAccountService();

    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    Logger::LogInfo("loggedIn=%d pid=%u", accountService->IsLoggedIn(), accountService->GetPid());
    RefreshView();
    m_multiList->ResetPointerInputState();
}

void UIScene_NetworkMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_NetworkMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_NetworkMenu *_this = (UIScene_NetworkMenu *) context;

    switch (id)
    {
        case s_networkMenuLoginId: {
            _this->Login();
            break;
        }

        case s_networkMenuFriendsId: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_FriendsMenu);
            break;
        }

        case s_networkMenuInvitesId: {
            Everstead::GetInstance()->GetUiController()->OpenScene(eUIScene_InvitationMenu);
            break;
        }

        case s_networkMenuLogoutId: {
            _this->Logout();
            break;
        }

        case s_networkMenuBackId: {
            Everstead::GetInstance()->GetUiController()->CloseScene();
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_NetworkMenu::RefreshView()
{
    AccountService *accountService = Everstead::GetInstance()->GetAccountService();
    bool loggedIn                  = accountService->IsLoggedIn();

    m_multiList->SetItemVisible(s_networkMenuUsernameLabelId, !loggedIn);
    m_multiList->SetItemVisible(s_networkMenuUsernameId, !loggedIn);
    m_multiList->SetItemVisible(s_networkMenuPasswordLabelId, !loggedIn);
    m_multiList->SetItemVisible(s_networkMenuPasswordId, !loggedIn);
    m_multiList->SetItemVisible(s_networkMenuLoginId, !loggedIn);
    m_multiList->SetItemVisible(s_networkMenuFriendsId, loggedIn);
    m_multiList->SetItemVisible(s_networkMenuInvitesId, loggedIn);
    m_multiList->SetItemVisible(s_networkMenuLogoutId, loggedIn);

    if (m_statusText.empty())
    {
        if (loggedIn)
        {
            m_multiList->SetLabelText(s_networkMenuStatusId, Everstead::GetInstance()->GetString("account.logged_in_as") + L" " +
                                                                     StringUtils::ToUtf16(accountService->GetUsername()));
        }
        else
        {
            m_multiList->SetLabelText(s_networkMenuStatusId, Everstead::GetInstance()->GetString("account.not_logged_in"));
        }
    }
    else
    {
        m_multiList->SetLabelText(s_networkMenuStatusId, m_statusText);
    }

    ResizeMenu();
}

void UIScene_NetworkMenu::Login()
{
    AccountService *accountService = Everstead::GetInstance()->GetAccountService();

    std::string username = StringUtils::ToUtf8(m_multiList->GetTextInputText(s_networkMenuUsernameId));
    std::string password = StringUtils::ToUtf8(m_multiList->GetTextInputText(s_networkMenuPasswordId));

    Logger::LogInfo("Network login started username=%s", username.c_str());

    accountService->Login(username, password);
    if (accountService->IsLoggedIn())
    {
        m_multiList->SetTextInputText(s_networkMenuPasswordId, L"");
    }

    m_statusText.clear();
    RefreshView();
}

void UIScene_NetworkMenu::Logout()
{
    AccountService *accountService = Everstead::GetInstance()->GetAccountService();
    if (!accountService->IsLoggedIn())
    {
        m_statusText.clear();
        RefreshView();
        return;
    }

    Logger::LogInfo("Network logout started pid=%u", accountService->GetPid());

    accountService->Logout();
    m_statusText.clear();
    RefreshView();
    m_multiList->ResetPointerInputState();
}

void UIScene_NetworkMenu::ResizeMenu()
{
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

#endif
