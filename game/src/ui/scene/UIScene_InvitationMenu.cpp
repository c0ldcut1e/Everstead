#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_InvitationMenu.h"

#include "../../core/Everstead.h"
#include "../../network/AccountService.h"
#include "../../network/MultiplayerSession.h"
#include "../../utils/StringUtils.h"
#include "../../utils/Time.h"
#include "../Screen.h"
#include "../control/UIControl_Label.h"
#include "../control/UIController.h"
#include "UIScene_ModalDialog.h"

UIScene_InvitationMenu::UIScene_InvitationMenu() : UIScene(eUIScene_InvitationMenu)
{
    m_groupRoot      = new UIGroup("Group_InvitationMenu");
    m_backgroundFill = new UIComponent_Fill(0x70000000);
    m_multiList      = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_entries        = {};
    m_pendingEntry   = {};
    m_inWorld        = false;
    m_lastRefreshMs  = 0;

    m_multiList->SetupControl(this);
    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(680.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetLabelRowWidth(680.0f);
    m_multiList->SetLabelRowHeight(30.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(10.0f);
    m_multiList->SetMaxVisibleElements(12);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_InvitationMenu::~UIScene_InvitationMenu()
{}

void UIScene_InvitationMenu::OnEnter()
{
    m_inWorld = Everstead::GetInstance()->GetLevel() != nullptr;
    if (m_inWorld && Everstead::GetInstance()->GetAccountService()->IsLoggedIn())
    {
        Everstead::GetInstance()->GetAccountService()->RequestRefreshFriends();
    }

    RefreshEntries();
    RefreshView();
    m_lastRefreshMs = Time::GetMilliseconds();
    m_multiList->ResetPointerInputState();
}

void UIScene_InvitationMenu::Tick()
{
    UIScene::Tick();

    bool inWorld = Everstead::GetInstance()->GetLevel() != nullptr;
    uint32_t now = Time::GetMilliseconds();
    if (m_inWorld != inWorld || now - m_lastRefreshMs >= s_invitationMenuRefreshInterval)
    {
        m_inWorld = inWorld;
        if (m_inWorld && Everstead::GetInstance()->GetAccountService()->IsLoggedIn())
        {
            Everstead::GetInstance()->GetAccountService()->RequestRefreshFriends();
        }

        RefreshEntries();
        RefreshView();
        m_lastRefreshMs = now;
    }
}

void UIScene_InvitationMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_InvitationMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_InvitationMenu *_this = (UIScene_InvitationMenu *) context;

    if (id == s_invitationMenuBackId)
    {
        Everstead::GetInstance()->GetUiController()->CloseScene();
        return;
    }

    if (id < s_invitationMenuEntryBaseId)
    {
        return;
    }

    uint32_t entryIndex = id - s_invitationMenuEntryBaseId;
    if (entryIndex >= _this->m_entries.size())
    {
        return;
    }

    _this->ConfirmEntry(_this->m_entries[entryIndex]);
}

void UIScene_InvitationMenu::HandleDialogButtonPressed(void *context, uint32_t id)
{
    UIScene_InvitationMenu *_this = (UIScene_InvitationMenu *) context;
    Everstead *everstead          = Everstead::GetInstance();
    const Entry &entry            = _this->m_pendingEntry;

    if (id == eDialogButton_SendInvite)
    {
        if (everstead->GetAccountService()->SendInvite(entry.pid, entry.sid))
        {
            everstead->GetUiController()->ShowNotification(everstead->GetString("invitation.sent_title"),
                                                           everstead->GetString("invitation.sent_message"), 2.0f);
        }
    }
    else if (id == eDialogButton_AcceptInvite)
    {
        if (!everstead->GetAccountService()->AcceptInvite(entry.inviteId))
        {
            return;
        }

        std::string worldName = StringUtils::ToUtf8(everstead->GetString("world.join.joined_world"));
        if (_this->m_inWorld)
        {
            everstead->JoinAfterLeavingWorld(entry.sid, worldName);
        }
        else
        {
            everstead->GetUiController()->CloseScene();
            everstead->StartJoinedWorld(entry.sid, worldName);
        }
    }
}

void UIScene_InvitationMenu::RefreshEntries()
{
    m_entries.clear();

    Everstead *everstead           = Everstead::GetInstance();
    AccountService *accountService = everstead->GetAccountService();
    if (!accountService->IsLoggedIn())
    {
        Entry entry;
        entry.type     = eEntryType_Message;
        entry.title    = everstead->GetString("invitation.not_signed_in");
        entry.message  = L"";
        entry.pid      = 0;
        entry.sid      = 0;
        entry.inviteId = "";
        m_entries.push_back(entry);
        return;
    }

    const std::vector<AccountService::Invite> &invites = accountService->GetIncomingInvites();
    for (size_t i = 0; i < invites.size(); i++)
    {
        const AccountService::Invite &inviteInfo = invites[i];
        Entry entry;
        entry.type     = eEntryType_Invite;
        entry.title    = StringUtils::ToUtf16(inviteInfo.sourceUsername);
        entry.message  = everstead->GetString("invitation.request");
        entry.pid      = inviteInfo.sourcePid;
        entry.sid      = inviteInfo.sid;
        entry.inviteId = inviteInfo.inviteId;
        m_entries.push_back(entry);
    }

    if (m_inWorld && !everstead->GetMultiplayerSession()->IsOnline())
    {
        Entry entry;
        entry.type     = eEntryType_Message;
        entry.title    = everstead->GetString("invitation.offline_world");
        entry.message  = L"";
        entry.pid      = 0;
        entry.sid      = 0;
        entry.inviteId = "";
        m_entries.push_back(entry);
        return;
    }

    if (m_inWorld)
    {
        size_t inviteCount                                 = m_entries.size();
        const std::vector<AccountService::Friend> &friends = accountService->GetFriends();
        for (size_t i = 0; i < friends.size(); i++)
        {
            const AccountService::Friend &friendInfo = friends[i];
            if (IsFriendOffline(friendInfo))
            {
                continue;
            }

            Entry entry;
            entry.type     = eEntryType_Friend;
            entry.title    = StringUtils::ToUtf16(friendInfo.username);
            entry.message  = L"";
            entry.pid      = friendInfo.pid;
            entry.sid      = everstead->GetMultiplayerSession()->GetCurrentSession().sid;
            entry.inviteId = "";
            m_entries.push_back(entry);
        }

        if (m_entries.size() == inviteCount)
        {
            Entry entry;
            entry.type     = eEntryType_Message;
            entry.title    = everstead->GetString("invitation.no_online_friends");
            entry.message  = L"";
            entry.pid      = 0;
            entry.sid      = 0;
            entry.inviteId = "";
            m_entries.push_back(entry);
        }

        return;
    }

    if (m_entries.empty())
    {
        Entry entry;
        entry.type     = eEntryType_Message;
        entry.title    = everstead->GetString("invitation.no_invitations");
        entry.message  = L"";
        entry.pid      = 0;
        entry.sid      = 0;
        entry.inviteId = "";
        m_entries.push_back(entry);
    }
}

void UIScene_InvitationMenu::RefreshView()
{
    Everstead *everstead = Everstead::GetInstance();
    m_multiList->ClearEntries();

    UIControl_Label *title = m_multiList->AddNewLabel(s_invitationMenuTitleId, m_inWorld ? everstead->GetString("invitation.invite_friends")
                                                                                         : everstead->GetString("invitation.invites"));
    title->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);

    for (size_t i = 0; i < m_entries.size(); i++)
    {
        const Entry &entry = m_entries[i];
        if (entry.type == eEntryType_Message)
        {
            UIControl_Label *message = m_multiList->AddNewLabel(s_invitationMenuMessageId, entry.title);
            message->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);
            continue;
        }

        std::wstring label = entry.title;
        if (!entry.message.empty())
        {
            label += L" - " + entry.message;
        }

        m_multiList->AddNewButton(s_invitationMenuEntryBaseId + (uint32_t) i, label);
    }

    m_multiList->AddNewSpacer(0, 20.0f);
    m_multiList->AddNewButton(s_invitationMenuBackId, everstead->GetString("button.back"));
    ResizeMenu();
}

bool UIScene_InvitationMenu::IsFriendOffline(const AccountService::Friend &friendInfo) const
{
    return !friendInfo.online || StringUtils::EqualsIgnoreCase(friendInfo.state, "offline");
}

void UIScene_InvitationMenu::ConfirmEntry(const Entry &entry)
{
    if (entry.type == eEntryType_Friend)
    {
        ConfirmFriendInvite(entry);
    }
    else if (entry.type == eEntryType_Invite)
    {
        ConfirmInviteAccept(entry);
    }
}

void UIScene_InvitationMenu::ConfirmFriendInvite(const Entry &entry)
{
    Everstead *everstead = Everstead::GetInstance();
    m_pendingEntry       = entry;

    UIScene_ModalDialog::Dialog dialog;
    dialog.title    = everstead->GetString("invitation.send_title");
    dialog.message  = everstead->GetString("invitation.invite_prefix") + L" " + entry.title + L" " + everstead->GetString("invitation.invite_suffix");
    dialog.callback = HandleDialogButtonPressed;
    dialog.context  = this;

    UIScene_ModalDialog::Button sendButton;
    sendButton.id    = eDialogButton_SendInvite;
    sendButton.label = everstead->GetString("invitation.invite");
    dialog.buttons.push_back(sendButton);

    UIScene_ModalDialog::Button cancelButton;
    cancelButton.id    = eDialogButton_Cancel;
    cancelButton.label = everstead->GetString("button.cancel");
    dialog.buttons.push_back(cancelButton);

    everstead->GetUiController()->ShowModal(dialog);
}

void UIScene_InvitationMenu::ConfirmInviteAccept(const Entry &entry)
{
    Everstead *everstead = Everstead::GetInstance();
    m_pendingEntry       = entry;

    UIScene_ModalDialog::Dialog dialog;
    dialog.title    = everstead->GetString("invitation.accept_title");
    dialog.message  = everstead->GetString(m_inWorld ? "invitation.leave_prefix" : "invitation.accept_prefix") + L" " + entry.title + L"?";
    dialog.callback = HandleDialogButtonPressed;
    dialog.context  = this;

    UIScene_ModalDialog::Button acceptButton;
    acceptButton.id    = eDialogButton_AcceptInvite;
    acceptButton.label = everstead->GetString("button.accept");
    dialog.buttons.push_back(acceptButton);

    UIScene_ModalDialog::Button cancelButton;
    cancelButton.id    = eDialogButton_Cancel;
    cancelButton.label = everstead->GetString("button.cancel");
    dialog.buttons.push_back(cancelButton);

    everstead->GetUiController()->ShowModal(dialog);
}

void UIScene_InvitationMenu::ResizeMenu()
{
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

#endif
