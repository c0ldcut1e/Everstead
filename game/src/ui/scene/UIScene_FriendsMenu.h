#pragma once

#include <cstdint>
#include <string>

#include "../../network/AccountService.h"
#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_FriendsMenu : public UIScene
{
public:
    UIScene_FriendsMenu();
    ~UIScene_FriendsMenu() override;

    void OnEnter() override;
    void Tick() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_friendsMenuTitleId               = 1;
    static const uint32_t s_friendsMenuUsernameLabelId       = 2;
    static const uint32_t s_friendsMenuUsernameInputId       = 3;
    static const uint32_t s_friendsMenuSendRequestId         = 4;
    static const uint32_t s_friendsMenuBackId                = 5;
    static const uint32_t s_friendsMenuEmptyLabelId          = 6;
    static const uint32_t s_friendsMenuFriendsButtonId       = 7;
    static const uint32_t s_friendsMenuIncomingButtonId      = 8;
    static const uint32_t s_friendsMenuOutgoingButtonId      = 9;
    static const uint32_t s_friendsMenuRemoveFriendId        = 10;
    static const uint32_t s_friendsMenuAcceptRequestId       = 11;
    static const uint32_t s_friendsMenuDenyRequestId         = 12;
    static const uint32_t s_friendsMenuCancelRequestId       = 13;
    static const uint32_t s_friendsMenuUserButtonBase        = 1000;
    static const uint32_t s_friendsMenuAutoRefreshIntervalMs = 3000;
    enum eVIEW : uint32_t
    {
        eView_Main                   = 0,
        eView_Friends                = 1,
        eView_FriendDetails          = 2,
        eView_IncomingRequests       = 3,
        eView_IncomingRequestDetails = 4,
        eView_OutgoingRequests       = 5,
        eView_OutgoingRequestDetails = 6,
    };

    static void HandleButtonPressed(void *context, uint32_t id);

    void RefreshView();
    void SendFriendRequest();
    void OpenMain();
    void OpenFriends();
    void OpenIncomingRequests();
    void OpenOutgoingRequests();
    void OpenFriendDetails(uint32_t index);
    void OpenIncomingRequestDetails(uint32_t index);
    void OpenOutgoingRequestDetails(uint32_t index);
    void GoBack();
    void RemoveFriend();
    void AcceptFriendRequest();
    void DenyFriendRequest();
    void CancelFriendRequest();
    std::wstring BuildUserLabel(const AccountService::Friend &friendInfo) const;
    bool IsFriendOffline(const AccountService::Friend &friendInfo) const;
    bool IsFriendJoinable(const AccountService::Friend &friendInfo) const;
    std::wstring BuildFriendStatusLabel(const AccountService::Friend &friendInfo) const;
    std::wstring BuildRequestDetailLabel(const AccountService::Friend &friendInfo) const;
    void ResizeMenu();

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    std::wstring m_usernameText;
    eVIEW m_view;
    uint32_t m_selectedIndex;
    uint32_t m_lastRefreshMs;
    uint32_t m_lastFriendsVersion;
};
