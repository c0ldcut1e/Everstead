#pragma once

#include <atomic>
#include <cstdint>
#include <future>
#include <string>
#include <vector>

#include "../utils/UniqueID.h"
#include "AccountService.h"
#include "NetworkResult.h"
#include "NetworkTypes.h"
#include "packet/DisconnectReason.h"

class ClientConnection;
class Connection;
class PeerNetwork;
class ServerLevel;

class MultiplayerSession
{
public:
    struct Player
    {
        Pid pid = 0;
        std::string username;
        bool local = false;
        bool host  = false;
    };

    struct SessionInfo
    {
        Sid sid     = 0;
        Pid hostPid = 0;
        std::string hostUsername;
        std::string name;
        std::string visibility;
        bool joinable         = false;
        bool friendsOfFriends = false;
        int maxPlayers        = 0;
        int currentPlayers    = 0;
        std::vector<Player> players;
    };

    enum eROOM_TASK_STATE
    {
        eRoomTask_None      = 0,
        eRoomTask_Pending   = 1,
        eRoomTask_Succeeded = 2,
        eRoomTask_Failed    = 3,
    };

    enum eROOM_TASK_STAGE
    {
        eRoomStage_FindingSession   = 0,
        eRoomStage_RequestingTicket = 1,
        eRoomStage_Connecting       = 2,
        eRoomStage_CreatingSession  = 3,
    };

    MultiplayerSession();
    ~MultiplayerSession();

    MultiplayerSession(const MultiplayerSession &)            = delete;
    MultiplayerSession &operator=(const MultiplayerSession &) = delete;

    void SetAccountService(AccountService *accountService);
    void SetServerLevel(ServerLevel *level);

    void Tick();

    bool BeginCreateRoom(const std::string &name, int maxPlayers, bool online);
    bool BeginJoinRoom(Sid sid);
    eROOM_TASK_STATE PollRoomTask();
    eROOM_TASK_STAGE GetRoomTaskStage() const;
    bool LeaveRoom();

    bool IsInSession() const;
    bool IsHostingSession() const;
    bool IsOnline() const;
    const SessionInfo &GetCurrentSession() const;
    const std::vector<Player> &GetPlayers() const;

    void RequestRefreshFriendSessions();
    uint32_t GetFriendSessionsVersion() const;
    const std::vector<SessionInfo> &GetFriendSessions() const;
    bool FindSession(Sid sid, SessionInfo *outPtr);
    void SetSelectedFriendSession(const SessionInfo &session);
    const SessionInfo &GetSelectedFriendSession() const;

    Pid GetClientPid() const;
    std::string GetClientUsername() const;
    UniqueID GetPlayerUniqueID(bool online) const;

    ClientConnection *TakeClientConnection();

private:
    struct JoinTicket
    {
        std::string ticket;
        std::string rendezvousHost;
        int rendezvousPort = 0;
        Pid hostPid        = 0;
    };

    struct RoomTaskResult
    {
        bool success = false;
        SessionInfo session;
        PeerNetwork *network   = nullptr;
        Connection *connection = nullptr;
        nlohmann::json document;
        std::string errorKey;
        std::string errorLog;
        NetworkResult result;
        bool appendMessage = false;
    };

    static const float s_heartbeatIntervalSeconds;
    static const float s_friendSessionRefreshSeconds;
    static inline const int s_leaveFlushTimeoutMs = 1000;
    static const int s_maxHeartbeatFailures       = 4;

    bool StartHosting(const std::string &name, int maxPlayers);
    RoomTaskResult RunJoinTask(Sid sid, std::vector<SessionInfo> knownSessions);
    RoomTaskResult RunHostTask(nlohmann::json body);
    bool FinishJoinRoom(RoomTaskResult *taskPtr);
    bool FinishHosting(RoomTaskResult *taskPtr);
    void FinishCreateRoom();
    void PublishSessionAsync(Sid sid, bool hosting);
    void StartLocalClientConnection();
    void StartRemoteClientConnection(Connection *connection);
    void AcceptHostConnections();
    void CleanupClosedChannels();
    void ClearChannels();
    void EndSession(eDISCONNECT_REASON reason);
    void TickHeartbeat();
    void TickPeerHeartbeat();
    NetworkResult SendHeartbeat(Sid sid, SessionInfo session);
    NetworkResult RequestJoinTicket(Sid sid, JoinTicket *outPtr);
    std::vector<SessionInfo> FetchFriendSessions();
    SessionInfo ReadSession(const nlohmann::json &document);
    void ReadSessionPlayers(SessionInfo *sessionPtr);
    void ClearCurrentSession();
    void RebuildLocalPlayers();
    ApiClient *GetApiClient() const;

    AccountService *m_accountService;
    ServerLevel *m_serverLevel;
    ClientConnection *m_clientConnection;
    PeerNetwork *m_peerNetwork;
    std::vector<Connection *> m_hostConnections;

    SessionInfo m_currentSession;
    SessionInfo m_selectedFriendSession;
    std::vector<SessionInfo> m_friendSessions;
    std::vector<Player> m_players;

    std::future<std::vector<SessionInfo>> m_friendSessionsFuture;
    std::future<NetworkResult> m_heartbeatFuture;
    std::future<RoomTaskResult> m_roomTaskFuture;
    std::future<void> m_publishFuture;
    std::atomic<int> m_roomTaskStage;
    std::atomic<bool> m_roomTaskCancelled;
    bool m_roomTaskHosting;
    bool m_roomTaskReady;

    uint32_t m_friendSessionsVersion;
    float m_heartbeatTimer;
    int m_heartbeatFailures;
    bool m_inSession;
    bool m_hostingSession;
    bool m_online;
};
