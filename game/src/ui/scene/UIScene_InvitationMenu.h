#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../network/AccountService.h"
#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_InvitationMenu : public UIScene
{
public:
    UIScene_InvitationMenu();
    ~UIScene_InvitationMenu() override;

    void OnEnter() override;
    void Tick() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    enum eENTRY_TYPE : uint8_t
    {
        eEntryType_Friend  = 0,
        eEntryType_Invite  = 1,
        eEntryType_Message = 2,
    };

    struct Entry
    {
        eENTRY_TYPE type;
        std::wstring title;
        std::wstring message;
        Pid pid;
        uint64_t sid;
        std::string inviteId;
    };

    static const uint32_t s_invitationMenuTitleId         = 1;
    static const uint32_t s_invitationMenuBackId          = 2;
    static const uint32_t s_invitationMenuMessageId       = 3;
    static const uint32_t s_invitationMenuEntryBaseId     = 100;
    static const uint32_t s_invitationMenuRefreshInterval = 3000;

    enum eDIALOG_BUTTON : uint32_t
    {
        eDialogButton_Cancel       = 0,
        eDialogButton_SendInvite   = 1,
        eDialogButton_AcceptInvite = 2,
    };

    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleDialogButtonPressed(void *context, uint32_t id);

    void RefreshEntries();
    void RefreshView();
    bool IsFriendOffline(const AccountService::Friend &friendInfo) const;
    void ConfirmEntry(const Entry &entry);
    void ConfirmFriendInvite(const Entry &entry);
    void ConfirmInviteAccept(const Entry &entry);
    void ResizeMenu();

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    std::vector<Entry> m_entries;
    Entry m_pendingEntry;
    bool m_inWorld;
    uint32_t m_lastRefreshMs;
};
