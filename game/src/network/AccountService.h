#pragma once

#include <chrono>
#include <cstdint>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

#include "../entity/player/Player.h"
#include "../io/File.h"
#include "ApiClient.h"
#include "NetworkResult.h"
#include "NetworkTypes.h"

class AccountService
{
public:
    struct Friend
    {
        Pid pid = 0;
        std::string requestId;
        std::string username;
        std::string state;
        bool joinable     = false;
        Sid currentSid    = 0;
        bool online       = false;
        int expiresInDays = 0;
    };

    struct Invite
    {
        std::string inviteId;
        Pid sourcePid = 0;
        Pid targetPid = 0;
        Sid sid       = 0;
        std::string sourceUsername;
        std::string targetUsername;
        int expiresInSeconds = 0;
    };

    AccountService();
    ~AccountService();

    AccountService(const AccountService &)            = delete;
    AccountService &operator=(const AccountService &) = delete;

    void Tick();

    ApiClient *GetApiClient();

    void Login(const std::string &username, const std::string &password);
    void Logout();
    bool IsLoggedIn() const;
    Pid GetPid() const;
    const std::string &GetUsername() const;

    uint32_t GetFriendsVersion() const;
    void RequestRefreshFriends();
    void RequestSendFriendRequest(const std::string &username);
    void RequestCancelFriendRequest(const std::string &requestId);
    void RequestAcceptFriendRequest(const std::string &requestId);
    void RequestDenyFriendRequest(const std::string &requestId);
    void RequestRemoveFriend(Pid pid);
    const std::vector<Friend> &GetFriends() const;
    const std::vector<Friend> &GetIncomingFriendRequests() const;
    const std::vector<Friend> &GetOutgoingFriendRequests() const;

    const std::vector<Invite> &GetIncomingInvites() const;
    bool SendInvite(Pid targetPid, Sid sid);
    bool AcceptInvite(const std::string &inviteId);

    void SetPresence(const std::string &state, bool joinable, Sid sid);

    void ApplyPlayerLook(Player *player);

    void ShowError(const std::wstring &message, const std::string &logText, const NetworkResult &result);

private:
    enum eFRIEND_OPERATION : uint8_t
    {
        eFriendOperation_Refresh       = 0,
        eFriendOperation_SendRequest   = 1,
        eFriendOperation_CancelRequest = 2,
        eFriendOperation_AcceptRequest = 3,
        eFriendOperation_DenyRequest   = 4,
        eFriendOperation_Remove        = 5,
    };

    struct FriendOperation
    {
        eFRIEND_OPERATION operation = eFriendOperation_Refresh;
        std::string text;
        Pid pid = 0;
    };

    struct FriendFetch
    {
        bool ok           = false;
        bool forcedLogout = false;
        NetworkResult result;
        std::vector<Friend> friends;
        std::vector<Friend> incoming;
        std::vector<Friend> outgoing;
    };

    struct FriendOperationResult
    {
        eFRIEND_OPERATION operation = eFriendOperation_Refresh;
        bool actionFailed           = false;
        NetworkResult actionResult;
        FriendFetch friends;
    };

    struct InviteFetch
    {
        bool ok           = false;
        bool forcedLogout = false;
        NetworkResult result;
        std::vector<Invite> incoming;
        std::vector<Invite> outgoing;
    };

    struct PlayerLook
    {
        std::string skinPath;
        ePLAYER_SKIN_MODEL skinModel = ePlayerSkinModel_Classic;
        std::string capePath;
    };

    struct SessionRefresh
    {
        NetworkResult result;
        std::string accessToken;
        std::string refreshToken;
    };

    static const float s_inviteRefreshIntervalSeconds;
    static const uint32_t s_loginDataMagic        = 0x93B75EC1;
    static const uint32_t s_loginDataVersion      = 1;
    static const size_t s_loginDataMaxPayloadSize = 65536;

    void TryAutoLogin();
    void ApplyAuthResponse(const nlohmann::json &document);
    bool IsForcedLogoutResult(const NetworkResult &result) const;
    void HandleForcedLogout(const char *action, const NetworkResult &result);
    void StartSessionRefresh();
    SessionRefresh RefreshSession(const std::string &refreshToken);
    void ResetSession();

    FriendFetch FetchFriends();
    Friend FetchFriend(Pid pid);
    FriendOperationResult RunFriendOperation(FriendOperation operation);
    void EnqueueFriendOperation(eFRIEND_OPERATION operation, const std::string &text, Pid pid);
    void ApplyFriendOperationError(eFRIEND_OPERATION operation, const NetworkResult &result);
    void ApplyFriendFetch(const FriendFetch &fetched);
    void PostPresence(const std::string &state, bool joinable, Sid sid);
    InviteFetch FetchInvites();
    PlayerLook FetchPlayerLook(const std::string &username);
    void NotifyNewIncomingInvites(const std::vector<Invite> &oldIncomingInvites);

    void SaveLoginData(const std::string &refreshToken);
    bool LoadLoginData(std::string *refreshTokenOutPtr);
    void DeleteLoginData();
    File GetLoginDataPath() const;
    void TransformLoginData(std::vector<uint8_t> *bytesPtr) const;
    void WriteU32(std::vector<uint8_t> *bufferPtr, uint32_t value) const;
    void WriteString(std::vector<uint8_t> *bufferPtr, const std::string &value) const;
    bool ReadU32(const std::vector<uint8_t> &buffer, size_t *positionPtr, uint32_t *valuePtr) const;
    bool ReadString(const std::vector<uint8_t> &buffer, size_t *positionPtr, std::string *valuePtr) const;

    std::wstring GetString(const std::string &key) const;

    ApiClient m_apiClient;
    bool m_loggedIn;
    Pid m_pid;
    std::string m_username;
    std::vector<Friend> m_friends;
    std::vector<Friend> m_incomingFriendRequests;
    std::vector<Friend> m_outgoingFriendRequests;
    std::vector<Invite> m_incomingInvites;
    std::vector<Invite> m_outgoingInvites;
    std::vector<FriendOperation> m_friendOperations;
    std::future<FriendOperationResult> m_friendOperationFuture;
    std::future<InviteFetch> m_inviteFuture;
    std::future<void> m_presenceFuture;
    std::future<SessionRefresh> m_sessionRefreshFuture;
    std::unordered_map<std::string, PlayerLook> m_playerLooks;
    std::vector<std::string> m_playerLookQueue;
    std::string m_playerLookUsername;
    std::future<PlayerLook> m_playerLookFuture;
    std::chrono::steady_clock::time_point m_sessionRefreshTime;
    std::string m_presenceState;
    bool m_presenceJoinable;
    Sid m_presenceSid;
    bool m_presenceDirty;
    float m_presenceTimer;
    uint32_t m_friendsVersion;
    float m_inviteRefreshTimer;
    bool m_inviteNotificationsReady;
    bool m_autoLoginAttempted;
};
