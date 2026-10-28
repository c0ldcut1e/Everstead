#ifndef EVERSTEAD_DEDICATED_SERVER

#include "AccountService.h"

#include <algorithm>
#include <chrono>

#include "../core/Everstead.h"
#include "../core/Logger.h"
#include "../io/FileSystem.h"
#include "../ui/control/UIController.h"
#include "../utils/StringUtils.h"
#include "../utils/Time.h"

static const char *s_defaultUsername = "Player";

const float AccountService::s_inviteRefreshIntervalSeconds = 5.0f;

AccountService::AccountService()
{
    m_loggedIn                 = false;
    m_pid                      = 0;
    m_username                 = s_defaultUsername;
    m_friends                  = {};
    m_incomingFriendRequests   = {};
    m_outgoingFriendRequests   = {};
    m_incomingInvites          = {};
    m_outgoingInvites          = {};
    m_friendOperations         = {};
    m_friendsVersion           = 0;
    m_inviteRefreshTimer       = 0.0f;
    m_inviteNotificationsReady = false;
    m_autoLoginAttempted       = false;
    m_presenceState            = "offline";
    m_presenceJoinable         = false;
    m_presenceSid              = 0;
    m_presenceDirty            = false;
    m_presenceTimer            = 0.0f;
    m_sessionRefreshTime       = std::chrono::steady_clock::now();
}

AccountService::~AccountService()
{
    if (m_presenceFuture.valid())
    {
        m_presenceFuture.wait();
    }

    if (m_loggedIn)
    {
        PostPresence("offline", false, 0);
    }
}

void AccountService::Tick()
{
    if (!m_loggedIn && !m_autoLoginAttempted)
    {
        m_autoLoginAttempted = true;
        TryAutoLogin();
    }

    if (!m_loggedIn)
    {
        return;
    }

    if (m_inviteFuture.valid() && m_inviteFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        InviteFetch fetched = m_inviteFuture.get();
        if (fetched.forcedLogout)
        {
            StartSessionRefresh();
        }

        if (fetched.ok)
        {
            std::vector<Invite> oldIncomingInvites = m_incomingInvites;
            m_incomingInvites                      = std::move(fetched.incoming);
            m_outgoingInvites                      = std::move(fetched.outgoing);
            NotifyNewIncomingInvites(oldIncomingInvites);
        }
    }

    if (m_friendOperationFuture.valid() && m_friendOperationFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        FriendOperationResult operationResult = m_friendOperationFuture.get();
        if (operationResult.actionFailed)
        {
            ApplyFriendOperationError(operationResult.operation, operationResult.actionResult);
        }
        else if (operationResult.friends.forcedLogout)
        {
            StartSessionRefresh();
        }
        else if (operationResult.friends.ok)
        {
            ApplyFriendFetch(operationResult.friends);
        }
    }

    if (!m_friendOperationFuture.valid() && !m_friendOperations.empty())
    {
        FriendOperation nextOperation = m_friendOperations[0];
        m_friendOperations.erase(m_friendOperations.begin());
        m_friendOperationFuture = std::async(std::launch::async, &AccountService::RunFriendOperation, this, nextOperation);
    }

    if (m_sessionRefreshFuture.valid() && m_sessionRefreshFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        SessionRefresh refreshed = m_sessionRefreshFuture.get();
        if (IsForcedLogoutResult(refreshed.result))
        {
            HandleForcedLogout("Refresh session", refreshed.result);
            return;
        }

        if (refreshed.result.IsFailure())
        {
            Logger::LogError("Session refresh failed code=%s", refreshed.result.Format().c_str());
        }
        else
        {
            m_apiClient.SetAccessToken(refreshed.accessToken);
            SaveLoginData(refreshed.refreshToken);
        }
    }

    // We refresh well before the access token expires since every API call fails once it does
    if (std::chrono::steady_clock::now() - m_sessionRefreshTime >= std::chrono::minutes(30))
    {
        StartSessionRefresh();
    }

    if (m_playerLookFuture.valid() && m_playerLookFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        m_playerLooks[m_playerLookUsername] = m_playerLookFuture.get();
    }

    if (!m_playerLookFuture.valid() && !m_playerLookQueue.empty())
    {
        m_playerLookUsername = m_playerLookQueue[0];
        m_playerLookQueue.erase(m_playerLookQueue.begin());
        m_playerLookFuture = std::async(std::launch::async, &AccountService::FetchPlayerLook, this, m_playerLookUsername);
    }

    constexpr float kPresenceHeartbeatSeconds = 30.0f;
    m_presenceTimer += (float) Time::GetDelta();
    if (m_presenceFuture.valid() && m_presenceFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        m_presenceFuture.get();
    }

    if (!m_presenceFuture.valid() && (m_presenceDirty || m_presenceTimer >= kPresenceHeartbeatSeconds))
    {
        m_presenceDirty  = false;
        m_presenceTimer  = 0.0f;
        m_presenceFuture = std::async(std::launch::async, &AccountService::PostPresence, this, m_presenceState, m_presenceJoinable, m_presenceSid);
    }

    m_inviteRefreshTimer += (float) Time::GetDelta();
    if (m_inviteFuture.valid() || m_inviteRefreshTimer < s_inviteRefreshIntervalSeconds)
    {
        return;
    }

    m_inviteRefreshTimer = 0.0f;
    m_inviteFuture       = std::async(std::launch::async, &AccountService::FetchInvites, this);
}

ApiClient *AccountService::GetApiClient()
{
    return &m_apiClient;
}

void AccountService::Login(const std::string &username, const std::string &password)
{
    if (username.empty())
    {
        ShowError(GetString("network.error.enter_username"), "Login blocked: empty username", NetworkResult());
        return;
    }

    if (password.empty())
    {
        ShowError(GetString("network.error.enter_password"), "Login blocked: empty password for username=" + username, NetworkResult());
        return;
    }

    nlohmann::json body;
    body["username"]  = username;
    body["password"]  = password;
    body["installId"] = m_apiClient.GetInstallId();

    nlohmann::json document;
    NetworkResult result = m_apiClient.Post("/account/login", body, &document);
    if (result.IsFailure())
    {
        ResetSession();
        ShowError(GetString("network.error.login_failed") + L": " + result.GetMessage(), "Login failed username=" + username, result);
        return;
    }

    ApplyAuthResponse(document);
    Logger::LogInfo("Login successful username=%s pid=%u", m_username.c_str(), m_pid);
}

void AccountService::Logout()
{
    if (!m_loggedIn)
    {
        return;
    }

    if (m_presenceFuture.valid())
    {
        m_presenceFuture.wait();
    }

    if (m_sessionRefreshFuture.valid())
    {
        SessionRefresh refreshed = m_sessionRefreshFuture.get();
        if (refreshed.result.IsSuccess())
        {
            m_apiClient.SetAccessToken(refreshed.accessToken);
        }
    }

    PostPresence("offline", false, 0);

    nlohmann::json document;
    NetworkResult result = m_apiClient.Post("/account/logout", nlohmann::json::object(), &document);
    if (result.IsFailure())
    {
        ShowError(GetString("network.error.logout_failed") + L": " + result.GetMessage(), "Logout failed", result);
    }

    DeleteLoginData();
    ResetSession();
    Logger::LogInfo("Logout complete");
}

bool AccountService::IsLoggedIn() const
{
    return m_loggedIn;
}

Pid AccountService::GetPid() const
{
    return m_pid;
}

const std::string &AccountService::GetUsername() const
{
    return m_username;
}

uint32_t AccountService::GetFriendsVersion() const
{
    return m_friendsVersion;
}

void AccountService::RequestRefreshFriends()
{
    EnqueueFriendOperation(eFriendOperation_Refresh, "", 0);
}

void AccountService::RequestSendFriendRequest(const std::string &username)
{
    if (username.empty())
    {
        ShowError(GetString("network.error.enter_username"), "Friend request blocked: empty name", NetworkResult());
        return;
    }

    EnqueueFriendOperation(eFriendOperation_SendRequest, username, 0);
}

void AccountService::RequestCancelFriendRequest(const std::string &requestId)
{
    EnqueueFriendOperation(eFriendOperation_CancelRequest, requestId, 0);
}

void AccountService::RequestAcceptFriendRequest(const std::string &requestId)
{
    EnqueueFriendOperation(eFriendOperation_AcceptRequest, requestId, 0);
}

void AccountService::RequestDenyFriendRequest(const std::string &requestId)
{
    EnqueueFriendOperation(eFriendOperation_DenyRequest, requestId, 0);
}

void AccountService::RequestRemoveFriend(Pid pid)
{
    EnqueueFriendOperation(eFriendOperation_Remove, "", pid);
}

const std::vector<AccountService::Friend> &AccountService::GetFriends() const
{
    return m_friends;
}

const std::vector<AccountService::Friend> &AccountService::GetIncomingFriendRequests() const
{
    return m_incomingFriendRequests;
}

const std::vector<AccountService::Friend> &AccountService::GetOutgoingFriendRequests() const
{
    return m_outgoingFriendRequests;
}

const std::vector<AccountService::Invite> &AccountService::GetIncomingInvites() const
{
    return m_incomingInvites;
}

bool AccountService::SendInvite(Pid targetPid, Sid sid)
{
    if (!m_loggedIn)
    {
        ShowError(GetString("network.error.log_into_account_first"), "Send invite blocked: not logged in",
                  NetworkResult(eErrorGroup_Client, eClientError_SignedOut));
        return false;
    }

    nlohmann::json body;
    body["targetPid"] = targetPid;
    body["sid"]       = std::to_string(sid);

    nlohmann::json document;
    NetworkResult result = m_apiClient.Post("/invites", body, &document);
    if (result.IsFailure())
    {
        ShowError(GetString("network.error.send_invite") + L": " + result.GetMessage(), "Send invite failed pid=" + std::to_string(targetPid),
                  result);
        return false;
    }

    return true;
}

bool AccountService::AcceptInvite(const std::string &inviteId)
{
    if (!m_loggedIn)
    {
        ShowError(GetString("network.error.log_into_account_first"), "Accept invite blocked: not logged in",
                  NetworkResult(eErrorGroup_Client, eClientError_SignedOut));
        return false;
    }

    nlohmann::json document;
    NetworkResult result = m_apiClient.Post("/invites/" + inviteId + "/accept", nlohmann::json::object(), &document);
    if (result.IsFailure())
    {
        ShowError(GetString("network.error.accept_invite") + L": " + result.GetMessage(), "Accept invite failed inviteId=" + inviteId, result);
        return false;
    }

    for (size_t i = 0; i < m_incomingInvites.size(); i++)
    {
        if (m_incomingInvites[i].inviteId == inviteId)
        {
            m_incomingInvites.erase(m_incomingInvites.begin() + i);
            break;
        }
    }

    return true;
}

void AccountService::SetPresence(const std::string &state, bool joinable, Sid sid)
{
    m_presenceState    = state;
    m_presenceJoinable = joinable;
    m_presenceSid      = sid;
    m_presenceDirty    = true;
}

void AccountService::ApplyPlayerLook(Player *player)
{
    std::string username = StringUtils::ToUtf8(player->GetName());
    auto found           = m_playerLooks.find(username);
    if (found != m_playerLooks.end())
    {
        if (!found->second.skinPath.empty())
        {
            player->SetSkin(found->second.skinPath, found->second.skinModel);
        }

        player->SetCape(found->second.capePath);
        return;
    }

    if (m_loggedIn && username != m_playerLookUsername &&
        std::find(m_playerLookQueue.begin(), m_playerLookQueue.end(), username) == m_playerLookQueue.end())
    {
        m_playerLookQueue.push_back(username);
    }
}

AccountService::PlayerLook AccountService::FetchPlayerLook(const std::string &username)
{
    PlayerLook look;
    nlohmann::json document;
    if (m_apiClient.Get("/users/name/" + username + "/look", &document).IsFailure())
    {
        return look;
    }

    if (document.value("skinModel", std::string()) == "slim")
    {
        look.skinModel = ePlayerSkinModel_Slim;
    }

    std::string skinHash = document.value("skinHash", std::string());
    if (!skinHash.empty())
    {
        File skinFile = FileSystem::GetUserDirectory().GetChild("skins").GetChild(skinHash + ".png");
        if (skinFile.Exists() || (skinFile.GetParent().CreateDirectories() && skinFile.WriteAllBytes(document["skin"].get<std::vector<uint8_t>>())))
        {
            look.skinPath = skinFile.GetPath();
        }
    }

    std::string cape = document.value("cape", std::string());
    if (!cape.empty())
    {
        look.capePath = "assets/texture/models/player/capes/" + cape.substr(cape.find(':') + 1) + ".png";
    }

    return look;
}

void AccountService::PostPresence(const std::string &state, bool joinable, Sid sid)
{
    nlohmann::json body;
    body["state"]      = state;
    body["joinable"]   = joinable;
    body["currentSid"] = std::to_string(sid);

    nlohmann::json document;
    NetworkResult result = m_apiClient.Post("/presence", body, &document);
    if (result.IsFailure())
    {
        Logger::LogError("Publish presence failed code=%s", result.Format().c_str());
    }
}

void AccountService::ShowError(const std::wstring &message, const std::string &logText, const NetworkResult &result)
{
    Logger::LogError("%s code=%s", logText.c_str(), result.IsFailure() ? result.Format().c_str() : "none");

    UIScene_ModalDialog::Dialog dialog;
    dialog.title = GetString("network.error.title");
    if (result.IsFailure())
    {
        dialog.title += L": ";
        dialog.title += StringUtils::ToUtf16(result.Format());
    }

    dialog.message = message;

    UIScene_ModalDialog::Button button;
    button.label = GetString("button.ok");
    dialog.buttons.push_back(button);

    Everstead::GetInstance()->GetUiController()->ShowModal(dialog);
}

void AccountService::TryAutoLogin()
{
    std::string refreshToken;
    if (!LoadLoginData(&refreshToken))
    {
        return;
    }

    Logger::LogInfo("Network auto login started");

    nlohmann::json body;
    body["refreshToken"] = refreshToken;

    nlohmann::json document;
    NetworkResult result = m_apiClient.Post("/account/refresh", body, &document);
    if (result.IsFailure())
    {
        ResetSession();
        if (IsForcedLogoutResult(result))
        {
            DeleteLoginData();
        }

        Logger::LogError("Auto login failed code=%s", result.Format().c_str());
        return;
    }

    ApplyAuthResponse(document);
    Logger::LogInfo("Auto login successful username=%s pid=%u", m_username.c_str(), m_pid);
}

void AccountService::ApplyAuthResponse(const nlohmann::json &document)
{
    m_apiClient.SetAccessToken(document.value("accessToken", std::string()));

    nlohmann::json account = document.value("account", nlohmann::json::object());
    m_pid                  = account.value("pid", 0);
    m_username             = account.value("username", std::string(s_defaultUsername));
    m_loggedIn             = true;

    SaveLoginData(document.value("refreshToken", std::string()));
    SetPresence("online", false, 0);
    RequestRefreshFriends();

    m_inviteRefreshTimer = s_inviteRefreshIntervalSeconds;
    m_sessionRefreshTime = std::chrono::steady_clock::now();
}

bool AccountService::IsForcedLogoutResult(const NetworkResult &result) const
{
    if (result.GetGroup() == eErrorGroup_Auth)
    {
        return true;
    }

    return result.GetGroup() == eErrorGroup_Account && (result.GetCode() == 5 || result.GetCode() == 6);
}

void AccountService::HandleForcedLogout(const char *action, const NetworkResult &result)
{
    DeleteLoginData();
    ResetSession();

    Everstead::GetInstance()->GetUiController()->ShowNotification(GetString("network.logged_out"), result.GetMessage(), 4.0f);

    Logger::LogError("%s forced a logout code=%s", action, result.Format().c_str());
}

void AccountService::StartSessionRefresh()
{
    if (m_sessionRefreshFuture.valid())
    {
        return;
    }

    std::string refreshToken;
    if (!LoadLoginData(&refreshToken))
    {
        ResetSession();
        return;
    }

    m_sessionRefreshTime   = std::chrono::steady_clock::now();
    m_sessionRefreshFuture = std::async(std::launch::async, &AccountService::RefreshSession, this, refreshToken);
}

AccountService::SessionRefresh AccountService::RefreshSession(const std::string &refreshToken)
{
    nlohmann::json body;
    body["refreshToken"] = refreshToken;

    nlohmann::json document;
    SessionRefresh refreshed;
    refreshed.result       = m_apiClient.Post("/account/refresh", body, &document);
    refreshed.accessToken  = document.value("accessToken", std::string());
    refreshed.refreshToken = document.value("refreshToken", std::string());
    return refreshed;
}

void AccountService::ResetSession()
{
    if (m_sessionRefreshFuture.valid())
    {
        m_sessionRefreshFuture.get();
    }

    m_apiClient.SetAccessToken("");
    m_loggedIn = false;
    m_pid      = 0;
    m_username = s_defaultUsername;
    m_friends.clear();
    m_incomingFriendRequests.clear();
    m_outgoingFriendRequests.clear();
    m_friendOperations.clear();
    m_friendsVersion++;
    m_incomingInvites.clear();
    m_outgoingInvites.clear();
    m_inviteNotificationsReady = false;
    m_inviteRefreshTimer       = 0.0f;
}

AccountService::FriendFetch AccountService::FetchFriends()
{
    FriendFetch fetched;

    nlohmann::json friendsDocument;
    fetched.result = m_apiClient.Get("/friends", &friendsDocument);
    if (fetched.result.IsFailure())
    {
        fetched.forcedLogout = IsForcedLogoutResult(fetched.result);
        return fetched;
    }

    nlohmann::json requestsDocument;
    fetched.result = m_apiClient.Get("/friends/requests", &requestsDocument);
    if (fetched.result.IsFailure())
    {
        fetched.forcedLogout = IsForcedLogoutResult(fetched.result);
        return fetched;
    }

    nlohmann::json friendPids = friendsDocument.value("friends", nlohmann::json::array());
    for (size_t i = 0; i < friendPids.size(); i++)
    {
        fetched.friends.push_back(FetchFriend(friendPids[i].get<Pid>()));
    }

    const char *directions[] = {"incoming", "outgoing"};
    for (size_t direction = 0; direction < 2; direction++)
    {
        nlohmann::json requests = requestsDocument.value(directions[direction], nlohmann::json::array());
        for (size_t i = 0; i < requests.size(); i++)
        {
            const nlohmann::json &entry = requests[i];
            bool incoming               = direction == 0;

            Friend request;
            request.requestId     = entry.value("requestId", std::string());
            request.pid           = incoming ? entry.value("sourcePid", 0) : entry.value("targetPid", 0);
            request.username      = incoming ? entry.value("sourceUsername", std::string()) : entry.value("targetUsername", std::string());
            request.expiresInDays = entry.value("expiresInDays", 0);

            if (incoming)
            {
                fetched.incoming.push_back(request);
            }
            else
            {
                fetched.outgoing.push_back(request);
            }
        }
    }

    fetched.ok = true;
    return fetched;
}

AccountService::Friend AccountService::FetchFriend(Pid pid)
{
    Friend found;
    found.pid = pid;

    nlohmann::json document;
    if (m_apiClient.Get("/users/" + std::to_string(pid), &document).IsFailure())
    {
        return found;
    }

    found.username = document.value("username", std::string());
    found.state    = document.value("state", std::string());
    found.joinable = document.value("joinable", false);
    found.online   = !found.state.empty() && found.state != "offline";

    std::string currentSid = document.value("currentSid", std::string("0"));
    found.currentSid       = strtoull(currentSid.c_str(), nullptr, 10);
    return found;
}

AccountService::FriendOperationResult AccountService::RunFriendOperation(FriendOperation operation)
{
    FriendOperationResult operationResult;
    operationResult.operation = operation.operation;

    nlohmann::json document;
    switch (operation.operation)
    {
        case eFriendOperation_SendRequest: {
            nlohmann::json user;
            operationResult.actionResult = m_apiClient.Get("/users/name/" + operation.text, &user);
            if (operationResult.actionResult.IsSuccess())
            {
                nlohmann::json body;
                body["targetPid"]            = user.value("pid", 0);
                operationResult.actionResult = m_apiClient.Post("/friends/requests", body, &document);
            }

            break;
        }

        case eFriendOperation_CancelRequest: {
            operationResult.actionResult = m_apiClient.Delete("/friends/requests/" + operation.text);
            break;
        }

        case eFriendOperation_AcceptRequest: {
            operationResult.actionResult = m_apiClient.Post("/friends/requests/" + operation.text + "/accept", nlohmann::json::object(), nullptr);
            break;
        }

        case eFriendOperation_DenyRequest: {
            operationResult.actionResult = m_apiClient.Post("/friends/requests/" + operation.text + "/deny", nlohmann::json::object(), nullptr);
            break;
        }

        case eFriendOperation_Remove: {
            operationResult.actionResult = m_apiClient.Delete("/friends/" + std::to_string(operation.pid));
            break;
        }

        default: {
            break;
        }
    }

    if (operationResult.actionResult.IsFailure())
    {
        operationResult.actionFailed = true;
        return operationResult;
    }

    operationResult.friends = FetchFriends();
    return operationResult;
}

void AccountService::EnqueueFriendOperation(eFRIEND_OPERATION operation, const std::string &text, Pid pid)
{
    if (!m_loggedIn)
    {
        ShowError(GetString("network.error.log_into_account_first"), "Friend action blocked: not logged in",
                  NetworkResult(eErrorGroup_Client, eClientError_SignedOut));
        return;
    }

    FriendOperation queued;
    queued.operation = operation;
    queued.text      = text;
    queued.pid       = pid;
    m_friendOperations.push_back(queued);
}

void AccountService::ApplyFriendOperationError(eFRIEND_OPERATION operation, const NetworkResult &result)
{
    if (IsForcedLogoutResult(result))
    {
        StartSessionRefresh();
        return;
    }

    std::wstring message = GetString("network.error.refresh_friends");
    switch (operation)
    {
        case eFriendOperation_SendRequest: {
            message = GetString("network.error.send_friend_request");
            break;
        }

        case eFriendOperation_CancelRequest: {
            message = GetString("network.error.cancel_friend_request");
            break;
        }

        case eFriendOperation_AcceptRequest: {
            message = GetString("network.error.accept_friend_request");
            break;
        }

        case eFriendOperation_DenyRequest: {
            message = GetString("network.error.deny_friend_request");
            break;
        }

        case eFriendOperation_Remove: {
            message = GetString("network.error.remove_friend");
            break;
        }

        default: {
            break;
        }
    }

    ShowError(message + L": " + result.GetMessage(), "Friend action failed", result);
}

void AccountService::ApplyFriendFetch(const FriendFetch &fetched)
{
    m_friends                = fetched.friends;
    m_incomingFriendRequests = fetched.incoming;
    m_outgoingFriendRequests = fetched.outgoing;
    m_friendsVersion++;
}

AccountService::InviteFetch AccountService::FetchInvites()
{
    InviteFetch fetched;

    nlohmann::json document;
    fetched.result = m_apiClient.Get("/invites", &document);
    if (fetched.result.IsFailure())
    {
        fetched.forcedLogout = IsForcedLogoutResult(fetched.result);
        return fetched;
    }

    const char *directions[] = {"incoming", "outgoing"};
    for (size_t direction = 0; direction < 2; direction++)
    {
        nlohmann::json invites = document.value(directions[direction], nlohmann::json::array());
        for (size_t i = 0; i < invites.size(); i++)
        {
            const nlohmann::json &entry = invites[i];

            Invite invite;
            invite.inviteId         = entry.value("inviteId", std::string());
            invite.sourcePid        = entry.value("sourcePid", 0);
            invite.targetPid        = entry.value("targetPid", 0);
            invite.sourceUsername   = entry.value("sourceUsername", std::string());
            invite.targetUsername   = entry.value("targetUsername", std::string());
            invite.expiresInSeconds = entry.value("expiresInSeconds", 0);

            std::string sid = entry.value("sid", std::string("0"));
            invite.sid      = strtoull(sid.c_str(), nullptr, 10);

            if (direction == 0)
            {
                fetched.incoming.push_back(invite);
            }
            else
            {
                fetched.outgoing.push_back(invite);
            }
        }
    }

    fetched.ok = true;
    return fetched;
}

void AccountService::NotifyNewIncomingInvites(const std::vector<Invite> &oldIncomingInvites)
{
    if (!m_inviteNotificationsReady)
    {
        m_inviteNotificationsReady = true;
        return;
    }

    for (size_t i = 0; i < m_incomingInvites.size(); i++)
    {
        const Invite &invite = m_incomingInvites[i];
        bool knownInvite     = false;
        for (size_t j = 0; j < oldIncomingInvites.size(); j++)
        {
            if (oldIncomingInvites[j].inviteId == invite.inviteId)
            {
                knownInvite = true;
                break;
            }
        }

        if (knownInvite)
        {
            continue;
        }

        Everstead::GetInstance()->GetUiController()->ShowNotification(
                GetString("invitation.title"),
                (invite.sourceUsername.empty() ? GetString("invitation.someone") : StringUtils::ToUtf16(invite.sourceUsername)) + L" " +
                        GetString("invitation.invited_you"),
                3.0f);
    }
}

void AccountService::SaveLoginData(const std::string &refreshToken)
{
    if (refreshToken.empty())
    {
        return;
    }

    std::vector<uint8_t> payload;
    WriteU32(&payload, m_pid);
    WriteString(&payload, m_username);
    WriteString(&payload, refreshToken);
    if (payload.size() > s_loginDataMaxPayloadSize)
    {
        Logger::LogError("Login data too large size=%zu", payload.size());
        return;
    }

    TransformLoginData(&payload);

    std::vector<uint8_t> bytes;
    WriteU32(&bytes, s_loginDataMagic);
    WriteU32(&bytes, s_loginDataVersion);
    WriteU32(&bytes, (uint32_t) payload.size());
    bytes.insert(bytes.end(), payload.begin(), payload.end());

    File path = GetLoginDataPath();
    if (!path.WriteAllBytes(bytes))
    {
        Logger::LogError("Could not write login data path=%s", path.GetPath().c_str());
        return;
    }

    path.SetOwnerOnlyPermissions();
}

bool AccountService::LoadLoginData(std::string *refreshTokenOutPtr)
{
    refreshTokenOutPtr->clear();

    File path = GetLoginDataPath();
    if (!path.Exists())
    {
        return false;
    }

    std::vector<uint8_t> bytes = path.ReadAllBytes();
    if (bytes.empty())
    {
        Logger::LogError("Could not read login data path=%s", path.GetPath().c_str());
        return false;
    }

    size_t position      = 0;
    uint32_t magic       = 0;
    uint32_t version     = 0;
    uint32_t payloadSize = 0;
    if (!ReadU32(bytes, &position, &magic) || !ReadU32(bytes, &position, &version) || !ReadU32(bytes, &position, &payloadSize) ||
        magic != s_loginDataMagic || version != s_loginDataVersion || payloadSize > s_loginDataMaxPayloadSize ||
        position + payloadSize > bytes.size())
    {
        Logger::LogError("Login data is invalid path=%s", path.GetPath().c_str());
        return false;
    }

    std::vector<uint8_t> payload(bytes.begin() + position, bytes.begin() + position + payloadSize);
    TransformLoginData(&payload);

    size_t payloadPosition = 0;
    uint32_t storedPid     = 0;
    std::string storedUsername;
    if (!ReadU32(payload, &payloadPosition, &storedPid) || !ReadString(payload, &payloadPosition, &storedUsername) ||
        !ReadString(payload, &payloadPosition, refreshTokenOutPtr) || refreshTokenOutPtr->empty())
    {
        Logger::LogError("Login data payload is invalid path=%s", path.GetPath().c_str());
        refreshTokenOutPtr->clear();
        return false;
    }

    return true;
}

void AccountService::DeleteLoginData()
{
    File path = GetLoginDataPath();
    if (!path.Exists())
    {
        return;
    }

    if (!path.Remove())
    {
        Logger::LogError("Could not delete login data path=%s", path.GetPath().c_str());
    }
}

File AccountService::GetLoginDataPath() const
{
    return FileSystem::GetUserDirectory().GetChild("login.dat");
}

void AccountService::TransformLoginData(std::vector<uint8_t> *bytesPtr) const
{
    for (size_t i = 0; i < bytesPtr->size(); i++)
    {
        uint8_t key    = (uint8_t) (0xA7 + (uint8_t) (i * 31));
        (*bytesPtr)[i] = (uint8_t) ((*bytesPtr)[i] ^ key);
    }
}

void AccountService::WriteU32(std::vector<uint8_t> *bufferPtr, uint32_t value) const
{
    bufferPtr->push_back((uint8_t) (value & 0xFF));
    bufferPtr->push_back((uint8_t) ((value >> 8) & 0xFF));
    bufferPtr->push_back((uint8_t) ((value >> 16) & 0xFF));
    bufferPtr->push_back((uint8_t) ((value >> 24) & 0xFF));
}

void AccountService::WriteString(std::vector<uint8_t> *bufferPtr, const std::string &value) const
{
    WriteU32(bufferPtr, (uint32_t) value.size());
    bufferPtr->insert(bufferPtr->end(), value.begin(), value.end());
}

bool AccountService::ReadU32(const std::vector<uint8_t> &buffer, size_t *positionPtr, uint32_t *valuePtr) const
{
    if (*positionPtr + 4 > buffer.size())
    {
        return false;
    }

    *valuePtr = (uint32_t) buffer[*positionPtr] | ((uint32_t) buffer[*positionPtr + 1] << 8) | ((uint32_t) buffer[*positionPtr + 2] << 16) |
                ((uint32_t) buffer[*positionPtr + 3] << 24);
    *positionPtr += 4;
    return true;
}

bool AccountService::ReadString(const std::vector<uint8_t> &buffer, size_t *positionPtr, std::string *valuePtr) const
{
    uint32_t length = 0;
    if (!ReadU32(buffer, positionPtr, &length) || *positionPtr + length > buffer.size())
    {
        return false;
    }

    valuePtr->assign((const char *) buffer.data() + *positionPtr, length);
    *positionPtr += length;
    return true;
}

std::wstring AccountService::GetString(const std::string &key) const
{
    return Everstead::GetInstance()->GetString(key);
}

#endif
