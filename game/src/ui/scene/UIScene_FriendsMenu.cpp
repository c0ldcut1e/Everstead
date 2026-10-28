#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_FriendsMenu.h"
#include "../../utils/Time.h"

#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../network/AccountService.h"
#include "../../utils/StringUtils.h"
#include "../Screen.h"

UIScene_FriendsMenu::UIScene_FriendsMenu() : UIScene(eUIScene_FriendsMenu)
{
    m_groupRoot          = new UIGroup("Group_FriendsMenu");
    m_backgroundFill     = new UIComponent_Fill(0x70000000);
    m_multiList          = {new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f)};
    m_usernameText       = {};
    m_view               = eView_Main;
    m_selectedIndex      = 0;
    m_lastRefreshMs      = 0;
    m_lastFriendsVersion = 0;
    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(680.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetTextInputRowWidth(680.0f);
    m_multiList->SetTextInputRowHeight(50.0f);
    m_multiList->SetLabelRowWidth(680.0f);
    m_multiList->SetLabelRowHeight(28.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(10.0f);
    m_multiList->SetMaxVisibleElements(13);

    m_backgroundFill->SetVisible(false);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);

    RefreshView();
}

UIScene_FriendsMenu::~UIScene_FriendsMenu()
{}

void UIScene_FriendsMenu::OnEnter()
{
    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    m_view                         = eView_Main;
    m_selectedIndex                = 0;
    AccountService *accountService = Everstead::GetInstance()->GetAccountService();
    m_lastFriendsVersion           = accountService->GetFriendsVersion();
    if (accountService->IsLoggedIn())
    {
        accountService->RequestRefreshFriends();
    }

    RefreshView();
    m_lastRefreshMs = Time::GetMilliseconds();
    m_multiList->ResetPointerInputState();
}

void UIScene_FriendsMenu::Tick()
{
    UIScene::Tick();

    AccountService *accountService = Everstead::GetInstance()->GetAccountService();
    if (!accountService->IsLoggedIn())
    {
        return;
    }

    if (accountService->GetFriendsVersion() != m_lastFriendsVersion)
    {
        m_lastFriendsVersion = accountService->GetFriendsVersion();
        RefreshView();
    }

    uint32_t now = Time::GetMilliseconds();
    if (now - m_lastRefreshMs < s_friendsMenuAutoRefreshIntervalMs)
    {
        return;
    }

    m_lastRefreshMs = now;
    m_usernameText  = m_multiList->GetTextInputText(s_friendsMenuUsernameInputId);
    accountService->RequestRefreshFriends();
}

void UIScene_FriendsMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    GoBack();
}

void UIScene_FriendsMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_FriendsMenu *_this = (UIScene_FriendsMenu *) context;

    if (id >= s_friendsMenuUserButtonBase)
    {
        uint32_t index = id - s_friendsMenuUserButtonBase;
        if (_this->m_view == eView_Friends)
        {
            _this->OpenFriendDetails(index);
        }
        else if (_this->m_view == eView_IncomingRequests)
        {
            _this->OpenIncomingRequestDetails(index);
        }
        else if (_this->m_view == eView_OutgoingRequests)
        {
            _this->OpenOutgoingRequestDetails(index);
        }

        return;
    }

    switch (id)
    {
        case s_friendsMenuSendRequestId: {
            _this->SendFriendRequest();
            break;
        }

        case s_friendsMenuFriendsButtonId: {
            _this->OpenFriends();
            break;
        }

        case s_friendsMenuIncomingButtonId: {
            _this->OpenIncomingRequests();
            break;
        }

        case s_friendsMenuOutgoingButtonId: {
            _this->OpenOutgoingRequests();
            break;
        }

        case s_friendsMenuRemoveFriendId: {
            _this->RemoveFriend();
            break;
        }

        case s_friendsMenuAcceptRequestId: {
            _this->AcceptFriendRequest();
            break;
        }

        case s_friendsMenuDenyRequestId: {
            _this->DenyFriendRequest();
            break;
        }

        case s_friendsMenuCancelRequestId: {
            _this->CancelFriendRequest();
            break;
        }

        case s_friendsMenuBackId: {
            _this->GoBack();
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_FriendsMenu::RefreshView()
{
    Everstead *everstead           = Everstead::GetInstance();
    AccountService *accountService = everstead->GetAccountService();
    m_usernameText                 = m_multiList->GetTextInputText(s_friendsMenuUsernameInputId);

    const std::vector<AccountService::Friend> &friends  = accountService->GetFriends();
    const std::vector<AccountService::Friend> &incoming = accountService->GetIncomingFriendRequests();
    const std::vector<AccountService::Friend> &outgoing = accountService->GetOutgoingFriendRequests();

    m_multiList->ClearEntries();

    if (!accountService->IsLoggedIn())
    {
        m_multiList->AddNewLabel(s_friendsMenuTitleId, everstead->GetString("friends.title"));
        m_multiList->AddNewLabel(s_friendsMenuEmptyLabelId, everstead->GetString("friends.log_into_account_first"));
        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    if (m_view == eView_Main)
    {
        m_multiList->AddNewLabel(s_friendsMenuTitleId, everstead->GetString("friends.title"));
        m_multiList->AddNewLabel(s_friendsMenuUsernameLabelId, everstead->GetString("account.username") + L":");
        m_multiList->AddNewTextInput(s_friendsMenuUsernameInputId);
        m_multiList->SetTextInputText(s_friendsMenuUsernameInputId, m_usernameText);
        m_multiList->AddNewButton(s_friendsMenuSendRequestId, everstead->GetString("friends.send_friend_request"));
        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuFriendsButtonId,
                                  everstead->GetString("friends.title") + L" (" + std::to_wstring(friends.size()) + L")");
        m_multiList->AddNewButton(s_friendsMenuIncomingButtonId,
                                  everstead->GetString("friends.incoming_requests") + L" (" + std::to_wstring(incoming.size()) + L")");
        m_multiList->AddNewButton(s_friendsMenuOutgoingButtonId,
                                  everstead->GetString("friends.outgoing_requests") + L" (" + std::to_wstring(outgoing.size()) + L")");
        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    if (m_view == eView_Friends)
    {
        m_multiList->AddNewLabel(s_friendsMenuTitleId, everstead->GetString("friends.title"));
        if (!friends.empty())
        {
            for (size_t i = 0; i < friends.size(); i++)
            {
                m_multiList->AddNewButton(s_friendsMenuUserButtonBase + (uint32_t) i, BuildUserLabel(friends[i]));
            }
        }

        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    if (m_view == eView_IncomingRequests)
    {
        m_multiList->AddNewLabel(s_friendsMenuTitleId, everstead->GetString("friends.incoming_requests"));
        if (incoming.empty())
        {
            m_multiList->AddNewLabel(s_friendsMenuEmptyLabelId, everstead->GetString("friends.no_incoming_requests"));
        }
        else
        {
            for (size_t i = 0; i < incoming.size(); i++)
            {
                m_multiList->AddNewButton(s_friendsMenuUserButtonBase + (uint32_t) i, BuildUserLabel(incoming[i]));
            }
        }

        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    if (m_view == eView_OutgoingRequests)
    {
        m_multiList->AddNewLabel(s_friendsMenuTitleId, everstead->GetString("friends.outgoing_requests"));
        if (outgoing.empty())
        {
            m_multiList->AddNewLabel(s_friendsMenuEmptyLabelId, everstead->GetString("friends.no_outgoing_requests"));
        }
        else
        {
            for (size_t i = 0; i < outgoing.size(); i++)
            {
                m_multiList->AddNewButton(s_friendsMenuUserButtonBase + (uint32_t) i, BuildUserLabel(outgoing[i]));
            }
        }

        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    if (m_view == eView_FriendDetails)
    {
        if (m_selectedIndex >= friends.size())
        {
            OpenFriends();
            return;
        }

        const AccountService::Friend &friendInfo = friends[m_selectedIndex];
        m_multiList->AddNewLabel(s_friendsMenuTitleId, BuildUserLabel(friendInfo));
        m_multiList->AddNewLabel(s_friendsMenuEmptyLabelId, BuildFriendStatusLabel(friendInfo));
        m_multiList->AddNewButton(s_friendsMenuRemoveFriendId, everstead->GetString("friends.remove_friend"));
        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    if (m_view == eView_IncomingRequestDetails)
    {
        if (m_selectedIndex >= incoming.size())
        {
            OpenIncomingRequests();
            return;
        }

        const AccountService::Friend &friendInfo = incoming[m_selectedIndex];
        m_multiList->AddNewLabel(s_friendsMenuTitleId, BuildUserLabel(friendInfo));
        m_multiList->AddNewLabel(s_friendsMenuEmptyLabelId, BuildRequestDetailLabel(friendInfo));
        m_multiList->AddNewButton(s_friendsMenuAcceptRequestId, everstead->GetString("button.accept"));
        m_multiList->AddNewButton(s_friendsMenuDenyRequestId, everstead->GetString("button.deny"));
        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
        return;
    }

    if (m_view == eView_OutgoingRequestDetails)
    {
        if (m_selectedIndex >= outgoing.size())
        {
            OpenOutgoingRequests();
            return;
        }

        const AccountService::Friend &friendInfo = outgoing[m_selectedIndex];
        m_multiList->AddNewLabel(s_friendsMenuTitleId, BuildUserLabel(friendInfo));
        m_multiList->AddNewLabel(s_friendsMenuEmptyLabelId, BuildRequestDetailLabel(friendInfo));
        m_multiList->AddNewButton(s_friendsMenuCancelRequestId, everstead->GetString("friends.cancel_request"));
        m_multiList->AddNewSpacer(0, 20.0f);
        m_multiList->AddNewButton(s_friendsMenuBackId, everstead->GetString("button.back"));
        ResizeMenu();
    }
}

void UIScene_FriendsMenu::SendFriendRequest()
{
    m_usernameText       = m_multiList->GetTextInputText(s_friendsMenuUsernameInputId);
    std::string username = StringUtils::Trim(StringUtils::ToUtf8(m_usernameText));
    Everstead::GetInstance()->GetAccountService()->RequestSendFriendRequest(username);
    m_usernameText.clear();
    m_multiList->SetTextInputText(s_friendsMenuUsernameInputId, L"");
    RefreshView();
}

void UIScene_FriendsMenu::OpenMain()
{
    m_view          = eView_Main;
    m_selectedIndex = 0;
    RefreshView();
}

void UIScene_FriendsMenu::OpenFriends()
{
    m_view          = eView_Friends;
    m_selectedIndex = 0;
    RefreshView();
}

void UIScene_FriendsMenu::OpenIncomingRequests()
{
    m_view          = eView_IncomingRequests;
    m_selectedIndex = 0;
    RefreshView();
}

void UIScene_FriendsMenu::OpenOutgoingRequests()
{
    m_view          = eView_OutgoingRequests;
    m_selectedIndex = 0;
    RefreshView();
}

void UIScene_FriendsMenu::OpenFriendDetails(uint32_t index)
{
    m_view          = eView_FriendDetails;
    m_selectedIndex = index;
    RefreshView();
}

void UIScene_FriendsMenu::OpenIncomingRequestDetails(uint32_t index)
{
    m_view          = eView_IncomingRequestDetails;
    m_selectedIndex = index;
    RefreshView();
}

void UIScene_FriendsMenu::OpenOutgoingRequestDetails(uint32_t index)
{
    m_view          = eView_OutgoingRequestDetails;
    m_selectedIndex = index;
    RefreshView();
}

void UIScene_FriendsMenu::GoBack()
{
    if (m_view == eView_FriendDetails)
    {
        OpenFriends();
        return;
    }

    if (m_view == eView_IncomingRequestDetails)
    {
        OpenIncomingRequests();
        return;
    }

    if (m_view == eView_OutgoingRequestDetails)
    {
        OpenOutgoingRequests();
        return;
    }

    if (m_view != eView_Main)
    {
        OpenMain();
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_FriendsMenu::RemoveFriend()
{
    const std::vector<AccountService::Friend> &friends = Everstead::GetInstance()->GetAccountService()->GetFriends();
    if (m_selectedIndex >= friends.size())
    {
        OpenFriends();
        return;
    }

    Everstead::GetInstance()->GetAccountService()->RequestRemoveFriend(friends[m_selectedIndex].pid);
    OpenFriends();
}

void UIScene_FriendsMenu::AcceptFriendRequest()
{
    const std::vector<AccountService::Friend> &incoming = Everstead::GetInstance()->GetAccountService()->GetIncomingFriendRequests();
    if (m_selectedIndex >= incoming.size())
    {
        OpenIncomingRequests();
        return;
    }

    Everstead::GetInstance()->GetAccountService()->RequestAcceptFriendRequest(incoming[m_selectedIndex].requestId);
    OpenIncomingRequests();
}

void UIScene_FriendsMenu::DenyFriendRequest()
{
    const std::vector<AccountService::Friend> &incoming = Everstead::GetInstance()->GetAccountService()->GetIncomingFriendRequests();
    if (m_selectedIndex >= incoming.size())
    {
        OpenIncomingRequests();
        return;
    }

    Everstead::GetInstance()->GetAccountService()->RequestDenyFriendRequest(incoming[m_selectedIndex].requestId);
    OpenIncomingRequests();
}

void UIScene_FriendsMenu::CancelFriendRequest()
{
    const std::vector<AccountService::Friend> &outgoing = Everstead::GetInstance()->GetAccountService()->GetOutgoingFriendRequests();
    if (m_selectedIndex >= outgoing.size())
    {
        OpenOutgoingRequests();
        return;
    }

    Everstead::GetInstance()->GetAccountService()->RequestCancelFriendRequest(outgoing[m_selectedIndex].requestId);
    OpenOutgoingRequests();
}

std::wstring UIScene_FriendsMenu::BuildUserLabel(const AccountService::Friend &friendInfo) const
{
    if (!friendInfo.username.empty())
    {
        return StringUtils::ToUtf16(friendInfo.username);
    }

    return Everstead::GetInstance()->GetString("friends.unknown_player");
}

bool UIScene_FriendsMenu::IsFriendOffline(const AccountService::Friend &friendInfo) const
{
    return !friendInfo.online || StringUtils::EqualsIgnoreCase(friendInfo.state, "offline");
}

bool UIScene_FriendsMenu::IsFriendJoinable(const AccountService::Friend &friendInfo) const
{
    return !IsFriendOffline(friendInfo) && friendInfo.joinable && friendInfo.currentSid != 0;
}

std::wstring UIScene_FriendsMenu::BuildFriendStatusLabel(const AccountService::Friend &friendInfo) const
{
    std::wstring label;
    if (IsFriendOffline(friendInfo))
    {
        label = Everstead::GetInstance()->GetString("friends.offline");
    }
    else if (IsFriendJoinable(friendInfo))
    {
        label = Everstead::GetInstance()->GetString("friends.in_online_world");
    }
    else if (StringUtils::EqualsIgnoreCase(friendInfo.state, "online"))
    {
        label = Everstead::GetInstance()->GetString("friends.online");
    }
    else if (StringUtils::EqualsIgnoreCase(friendInfo.state, "in online world"))
    {
        label = Everstead::GetInstance()->GetString("friends.in_online_world");
    }
    else
    {
        label = StringUtils::ToUtf16(friendInfo.state);
    }

    if (IsFriendJoinable(friendInfo))
    {
        label += L" - " + Everstead::GetInstance()->GetString("friends.joinable");
    }

    return label;
}

std::wstring UIScene_FriendsMenu::BuildRequestDetailLabel(const AccountService::Friend &friendInfo) const
{
    if (friendInfo.expiresInDays <= 0)
    {
        return Everstead::GetInstance()->GetString("friends.expires_soon");
    }

    std::wstring label = std::to_wstring(friendInfo.expiresInDays);
    label += L" ";
    label += friendInfo.expiresInDays == 1 ? Everstead::GetInstance()->GetString("friends.day_left")
                                           : Everstead::GetInstance()->GetString("friends.days_left");
    return label;
}

void UIScene_FriendsMenu::ResizeMenu()
{
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

#endif
