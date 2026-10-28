#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MultiplayerSession.h"

#include <chrono>
#include <cstdlib>
#include <mutex>
#include <thread>

#include "../core/Everstead.h"
#include "../core/EversteadServer.h"
#include "../core/LocalProfile.h"
#include "../core/Logger.h"
#include "../utils/Time.h"
#include "../world/ServerLevel.h"
#include "Connection.h"
#include "PeerNetwork.h"
#include "PlayerList.h"
#include "ServerConnection.h"
#include "listener/ClientConnection.h"
#include "listener/PendingConnection.h"
#include "packet/Packet.h"
#include "packet/c2s/DisconnectC2SPacket.h"
#include "packet/s2c/DisconnectS2CPacket.h"

static const char *s_offlineUsername = "Player";

const float MultiplayerSession::s_heartbeatIntervalSeconds    = 30.0f;
const float MultiplayerSession::s_friendSessionRefreshSeconds = 5.0f;

MultiplayerSession::MultiplayerSession()
{
    m_accountService        = nullptr;
    m_serverLevel           = nullptr;
    m_clientConnection      = nullptr;
    m_peerNetwork           = nullptr;
    m_hostConnections       = {};
    m_currentSession        = SessionInfo();
    m_selectedFriendSession = SessionInfo();
    m_friendSessions        = {};
    m_players               = {};
    m_roomTaskStage         = eRoomStage_FindingSession;
    m_roomTaskCancelled     = false;
    m_roomTaskHosting       = false;
    m_roomTaskReady         = false;
    m_friendSessionsVersion = 0;
    m_heartbeatTimer        = 0.0f;
    m_heartbeatFailures     = 0;
    m_inSession             = false;
    m_hostingSession        = false;
    m_online                = false;
}

MultiplayerSession::~MultiplayerSession()
{
    ClearChannels();
    delete m_clientConnection;
}

void MultiplayerSession::SetAccountService(AccountService *accountService)
{
    m_accountService = accountService;
}

void MultiplayerSession::SetServerLevel(ServerLevel *level)
{
    m_serverLevel = level;
}

void MultiplayerSession::Tick()
{
    if (m_friendSessionsFuture.valid() && m_friendSessionsFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        m_friendSessions = m_friendSessionsFuture.get();
        m_friendSessionsVersion++;
    }

    if (!m_inSession)
    {
        return;
    }

    if (m_hostingSession)
    {
        AcceptHostConnections();
    }

    CleanupClosedChannels();

    // Online sessions need an account, signing out ends them for the host and for joined players
    if (m_online && !m_accountService->IsLoggedIn())
    {
        EndSession(eDisconnectReason_SignedOut);
        return;
    }

    if (m_hostingSession && m_online)
    {
        TickHeartbeat();
    }
    else if (m_inSession && m_online)
    {
        TickPeerHeartbeat();
    }
}

bool MultiplayerSession::BeginCreateRoom(const std::string &name, int maxPlayers, bool online)
{
    ClearCurrentSession();
    m_online          = online && m_accountService->IsLoggedIn();
    m_roomTaskHosting = true;
    m_roomTaskReady   = false;
    m_roomTaskStage   = eRoomStage_CreatingSession;
    return StartHosting(name, maxPlayers);
}

bool MultiplayerSession::BeginJoinRoom(Sid sid)
{
    if (!m_accountService->IsLoggedIn())
    {
        m_accountService->ShowError(Everstead::GetInstance()->GetString("network.error.log_into_account_first"), "Join blocked: not logged in",
                                    NetworkResult(eErrorGroup_Client, eClientError_SignedOut));
        return false;
    }

    m_roomTaskHosting   = false;
    m_roomTaskReady     = false;
    m_roomTaskStage     = eRoomStage_FindingSession;
    m_roomTaskCancelled = false;
    m_roomTaskFuture    = std::async(std::launch::async, &MultiplayerSession::RunJoinTask, this, sid, m_friendSessions);
    return true;
}

MultiplayerSession::eROOM_TASK_STATE MultiplayerSession::PollRoomTask()
{
    if (m_roomTaskReady)
    {
        m_roomTaskReady = false;
        FinishCreateRoom();
        return eRoomTask_Succeeded;
    }

    if (!m_roomTaskFuture.valid())
    {
        return eRoomTask_None;
    }

    if (m_roomTaskFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        return eRoomTask_Pending;
    }

    RoomTaskResult task = m_roomTaskFuture.get();
    bool success        = m_roomTaskHosting ? FinishHosting(&task) : FinishJoinRoom(&task);
    return success ? eRoomTask_Succeeded : eRoomTask_Failed;
}

MultiplayerSession::eROOM_TASK_STAGE MultiplayerSession::GetRoomTaskStage() const
{
    return (eROOM_TASK_STAGE) m_roomTaskStage.load();
}

MultiplayerSession::RoomTaskResult MultiplayerSession::RunJoinTask(Sid sid, std::vector<SessionInfo> knownSessions)
{
    RoomTaskResult task;
    bool found = false;
    for (size_t i = 0; i < knownSessions.size(); i++)
    {
        if (knownSessions[i].sid == sid)
        {
            task.session = knownSessions[i];
            found        = true;
            break;
        }
    }

    if (!found)
    {
        nlohmann::json document;
        if (GetApiClient()->Get("/sessions/" + std::to_string(sid), &document).IsFailure())
        {
            task.errorKey = "network.error.session_not_found";
            task.errorLog = "Join failed: session not found sid=" + std::to_string(sid);
            task.result   = NetworkResult(eErrorGroup_Session, 1);
            return task;
        }

        task.session = ReadSession(document);
        ReadSessionPlayers(&task.session);
    }

    m_roomTaskStage = eRoomStage_RequestingTicket;
    JoinTicket ticket;
    task.result = RequestJoinTicket(sid, &ticket);
    if (task.result.IsFailure())
    {
        task.errorKey      = "network.error.join_failed";
        task.errorLog      = "Join failed: no ticket sid=" + std::to_string(sid);
        task.appendMessage = true;
        return task;
    }

    m_roomTaskStage        = eRoomStage_Connecting;
    PeerNetwork *network   = new PeerNetwork();
    Connection *connection = nullptr;
    if (network->StartJoin(ticket.rendezvousHost, ticket.rendezvousPort, ticket.ticket))
    {
        Pid pid = 0;
        while (!m_roomTaskCancelled && !network->HasFailed() && !network->PollAcceptedConnection(&connection, &pid))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (!connection)
    {
        delete network;
        task.errorKey = "network.error.join_failed";
        task.errorLog = "Join failed: no route to the host sid=" + std::to_string(sid);
        task.result   = NetworkResult(eErrorGroup_Client, eClientError_HostConnect);
        return task;
    }

    task.network    = network;
    task.connection = connection;
    task.success    = true;
    return task;
}

bool MultiplayerSession::FinishJoinRoom(RoomTaskResult *taskPtr)
{
    if (!taskPtr->success)
    {
        std::wstring message = Everstead::GetInstance()->GetString(taskPtr->errorKey);
        if (taskPtr->appendMessage)
        {
            message += L": " + taskPtr->result.GetMessage();
        }

        m_accountService->ShowError(message, taskPtr->errorLog, taskPtr->result);
        return false;
    }

    Sid sid          = taskPtr->session.sid;
    m_currentSession = taskPtr->session;
    m_inSession      = true;
    m_hostingSession = false;
    m_online         = true;
    m_heartbeatTimer = 0.0f;

    m_peerNetwork = taskPtr->network;
    StartRemoteClientConnection(taskPtr->connection);
    RebuildLocalPlayers();
    PublishSessionAsync(sid, false);

    Logger::LogInfo("Joined session sid=%llu hostPid=%u", (unsigned long long) sid, m_currentSession.hostPid);
    return true;
}

void MultiplayerSession::FinishCreateRoom()
{
    std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
    EversteadServer::GetInstance()->Start(m_serverLevel, m_currentSession.name, m_currentSession.sid, m_currentSession.maxPlayers);
    EversteadServer::GetInstance()->settings.hostPid = GetClientPid();
    StartLocalClientConnection();
}

void MultiplayerSession::PublishSessionAsync(Sid sid, bool hosting)
{
    m_accountService->SetPresence("playing", hosting, sid);
    if (hosting)
    {
        return;
    }

    if (m_publishFuture.valid())
    {
        m_publishFuture.wait();
    }

    std::string username = m_accountService->GetUsername();
    m_publishFuture      = std::async(std::launch::async,
                                      [this, sid, username]()
                                      {
                                     nlohmann::json body;
                                     body["username"] = username;
                                     body["state"]    = "ready";
                                     nlohmann::json document;
                                     GetApiClient()->Put("/sessions/" + std::to_string(sid) + "/peers/me", body, &document);
                                      });
}

bool MultiplayerSession::LeaveRoom()
{
    m_roomTaskReady     = false;
    m_roomTaskCancelled = true;
    if (m_roomTaskFuture.valid())
    {
        RoomTaskResult task = m_roomTaskFuture.get();
        delete task.network;
        delete task.connection;
    }

    if (!m_inSession)
    {
        return true;
    }

    std::vector<Connection *> flushing = m_hostConnections;
    if (!m_hostingSession && m_clientConnection)
    {
        flushing.push_back(m_clientConnection->GetConnection());
    }

    for (size_t i = 0; i < flushing.size(); i++)
    {
        if (m_hostingSession)
        {
            flushing[i]->QueueIncomingPacket(std::shared_ptr<Packet>(new DisconnectS2CPacket(eDisconnectReason_ServerStopping)));
            flushing[i]->CloseAfterFlush(eDisconnectReason_ServerStopping);
        }
        else
        {
            flushing[i]->Send(std::shared_ptr<Packet>(new DisconnectC2SPacket(eDisconnectReason_Closed)));
            flushing[i]->CloseAfterFlush(eDisconnectReason_Closed);
        }
    }

    // We wait for the network thread to write the disconnect so players see why they left instead of a dropped connection
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(s_leaveFlushTimeoutMs);
    while (m_peerNetwork && std::chrono::steady_clock::now() < deadline)
    {
        bool open = false;
        for (size_t i = 0; i < flushing.size(); i++)
        {
            open = open || flushing[i]->GetSocket()->IsOpen();
        }

        if (!open)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (m_online)
    {
        Sid sid = m_currentSession.sid;
        if (m_hostingSession)
        {
            GetApiClient()->Delete("/sessions/" + std::to_string(sid));
        }
        else
        {
            GetApiClient()->Delete("/sessions/" + std::to_string(sid) + "/peers/me");
        }

        m_accountService->SetPresence("online", false, 0);
    }

    {
        std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
        EversteadServer::GetInstance()->Stop();
    }

    ClearChannels();
    delete m_clientConnection;
    m_clientConnection = nullptr;

    ClearCurrentSession();
    return true;
}

bool MultiplayerSession::IsInSession() const
{
    return m_inSession;
}

bool MultiplayerSession::IsHostingSession() const
{
    return m_inSession && m_hostingSession;
}

bool MultiplayerSession::IsOnline() const
{
    return m_online;
}

const MultiplayerSession::SessionInfo &MultiplayerSession::GetCurrentSession() const
{
    return m_currentSession;
}

const std::vector<MultiplayerSession::Player> &MultiplayerSession::GetPlayers() const
{
    return m_players;
}

void MultiplayerSession::RequestRefreshFriendSessions()
{
    if (!m_accountService->IsLoggedIn() || m_friendSessionsFuture.valid())
    {
        return;
    }

    m_friendSessionsFuture = std::async(std::launch::async, &MultiplayerSession::FetchFriendSessions, this);
}

uint32_t MultiplayerSession::GetFriendSessionsVersion() const
{
    return m_friendSessionsVersion;
}

const std::vector<MultiplayerSession::SessionInfo> &MultiplayerSession::GetFriendSessions() const
{
    return m_friendSessions;
}

bool MultiplayerSession::FindSession(Sid sid, SessionInfo *outPtr)
{
    for (size_t i = 0; i < m_friendSessions.size(); i++)
    {
        if (m_friendSessions[i].sid == sid)
        {
            *outPtr = m_friendSessions[i];
            return true;
        }
    }

    nlohmann::json document;
    if (GetApiClient()->Get("/sessions/" + std::to_string(sid), &document).IsFailure())
    {
        return false;
    }

    *outPtr = ReadSession(document);
    ReadSessionPlayers(outPtr);
    return true;
}

void MultiplayerSession::SetSelectedFriendSession(const SessionInfo &session)
{
    m_selectedFriendSession = session;
}

const MultiplayerSession::SessionInfo &MultiplayerSession::GetSelectedFriendSession() const
{
    return m_selectedFriendSession;
}

Pid MultiplayerSession::GetClientPid() const
{
    if (!m_online)
    {
        return 0;
    }

    return m_accountService->GetPid();
}

std::string MultiplayerSession::GetClientUsername() const
{
    if (!m_online)
    {
        return s_offlineUsername;
    }

    return m_accountService->GetUsername();
}

UniqueID MultiplayerSession::GetPlayerUniqueID(bool online) const
{
    if (!online)
    {
        return LocalProfile::GetPlayerUniqueID();
    }

    return UniqueID::FromSeed(m_accountService->GetPid());
}

ClientConnection *MultiplayerSession::TakeClientConnection()
{
    ClientConnection *clientConnection = m_clientConnection;
    m_clientConnection                 = nullptr;
    return clientConnection;
}

bool MultiplayerSession::StartHosting(const std::string &name, int maxPlayers)
{
    m_currentSession.name             = name;
    m_currentSession.maxPlayers       = maxPlayers;
    m_currentSession.currentPlayers   = 1;
    m_currentSession.joinable         = true;
    m_currentSession.visibility       = "friends";
    m_currentSession.friendsOfFriends = m_serverLevel->AllowsFriendsOfFriends();
    m_currentSession.hostPid          = GetClientPid();
    m_currentSession.hostUsername     = GetClientUsername();
    m_currentSession.sid              = 1;
    m_inSession                       = true;
    m_hostingSession                  = true;
    m_heartbeatTimer                  = 0.0f;

    if (!m_online)
    {
        RebuildLocalPlayers();
        m_roomTaskReady = true;
        return true;
    }

    nlohmann::json body;
    body["name"]             = name;
    body["visibility"]       = m_currentSession.visibility;
    body["joinable"]         = true;
    body["friendsOfFriends"] = m_currentSession.friendsOfFriends;
    body["maxPlayers"]       = maxPlayers;
    body["currentPlayers"]   = 1;

    m_roomTaskFuture = std::async(std::launch::async, &MultiplayerSession::RunHostTask, this, body);
    return true;
}

MultiplayerSession::RoomTaskResult MultiplayerSession::RunHostTask(nlohmann::json body)
{
    RoomTaskResult task;
    task.result = GetApiClient()->Post("/sessions", body, &task.document);
    if (task.result.IsFailure())
    {
        return task;
    }

    task.network = new PeerNetwork();
    if (!task.network->StartHost(task.document.value("rendezvousHost", std::string()), task.document.value("rendezvousPort", 0),
                                 task.document.value("hostKey", std::string()), 0))
    {
        delete task.network;
        task.network = nullptr;
        GetApiClient()->Delete("/sessions/" + task.document.value("sid", std::string("0")));
        task.result = NetworkResult(eErrorGroup_Client, eClientError_HostConnect);
        return task;
    }

    task.success = true;
    return task;
}

bool MultiplayerSession::FinishHosting(RoomTaskResult *taskPtr)
{
    if (!taskPtr->success)
    {
        m_accountService->ShowError(Everstead::GetInstance()->GetString("network.error.host_failed") + L": " + taskPtr->result.GetMessage(),
                                    "Hosting failed: could not create the session", taskPtr->result);
        ClearCurrentSession();
        return false;
    }

    m_currentSession.sid = ReadSession(taskPtr->document).sid;
    m_peerNetwork        = taskPtr->network;

    PublishSessionAsync(m_currentSession.sid, true);
    RebuildLocalPlayers();

    Logger::LogInfo("Hosting session sid=%llu", (unsigned long long) m_currentSession.sid);
    FinishCreateRoom();
    return true;
}

void MultiplayerSession::StartLocalClientConnection()
{
    delete m_clientConnection;

    std::string name       = "local-" + m_currentSession.name;
    Connection *connection = new Connection(name, new Socket("socket-" + m_currentSession.name));
    EversteadServer::GetInstance()->GetConnection()->AddPendingConnection(new PendingConnection(connection, m_currentSession.sid, GetClientPid()));
    m_clientConnection = new ClientConnection(connection);
    m_clientConnection->BeginLogin(GetClientPid(), GetClientUsername(), GetPlayerUniqueID(m_online));
    EversteadServer::GetInstance()->GetConnection()->Tick();
    m_clientConnection->Tick();

    Logger::LogInfo("Local client connection started name=%s loggedIn=%d canEnterWorld=%d", name.c_str(), m_clientConnection->IsLoggedIn(),
                    m_clientConnection->CanEnterWorld());
}

void MultiplayerSession::StartRemoteClientConnection(Connection *connection)
{
    delete m_clientConnection;
    connection->SetName("client-" + m_currentSession.name);
    m_clientConnection = new ClientConnection(connection);
    m_clientConnection->BeginLogin(GetClientPid(), GetClientUsername(), GetPlayerUniqueID(m_online));

    Logger::LogInfo("Client connection started name=%s hostPid=%u sid=%llu", connection->GetName().c_str(), m_currentSession.hostPid,
                    (unsigned long long) m_currentSession.sid);
}

void MultiplayerSession::AcceptHostConnections()
{
    if (!m_peerNetwork)
    {
        return;
    }

    // Accepted connections stay queued in the peer network, so we skip a frame instead of waiting out a server tick
    std::unique_lock<std::mutex> lock(EversteadServer::GetInstance()->GetMutex(), std::try_to_lock);
    if (!lock.owns_lock())
    {
        return;
    }

    Connection *connection = nullptr;
    Pid pid                = 0;
    while (m_peerNetwork->PollAcceptedConnection(&connection, &pid))
    {
        EversteadServer::GetInstance()->GetConnection()->AddPendingConnection(new PendingConnection(connection, m_currentSession.sid, pid));
        m_hostConnections.push_back(connection);
        Logger::LogInfo("Host accepted a player connection name=%s", connection->GetName().c_str());
    }
}

void MultiplayerSession::CleanupClosedChannels()
{
    // This runs every frame, so we skip it while the server thread is mid tick instead of stalling the render thread
    std::unique_lock<std::mutex> lock(EversteadServer::GetInstance()->GetMutex(), std::try_to_lock);
    if (!lock.owns_lock())
    {
        return;
    }

    size_t i = 0;
    while (i < m_hostConnections.size())
    {
        Connection *connection = m_hostConnections[i];
        if (!connection->IsClosed() || connection->IsFlushing() || connection->GetReceivedPacketCount() > 0 ||
            EversteadServer::GetInstance()->GetConnection()->HasPendingConnection(connection) ||
            EversteadServer::GetInstance()->GetPlayerList()->HasConnection(connection))
        {
            i++;
            continue;
        }

        Logger::LogInfo("Host connection cleaned up name=%s", connection->GetName().c_str());
        m_peerNetwork->RemoveConnection(connection);
        delete connection;
        m_hostConnections.erase(m_hostConnections.begin() + (long) i);
    }
}

void MultiplayerSession::ClearChannels()
{
    delete m_peerNetwork;
    m_peerNetwork = nullptr;

    for (size_t i = 0; i < m_hostConnections.size(); i++)
    {
        delete m_hostConnections[i];
    }

    m_hostConnections.clear();
}

void MultiplayerSession::EndSession(eDISCONNECT_REASON reason)
{
    ClientConnection *clientConnection  = m_clientConnection;
    MultiPlayerLocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (!clientConnection && localPlayer)
    {
        clientConnection = localPlayer->GetClientConnection();
    }

    if (clientConnection)
    {
        clientConnection->ForceDisconnect(reason);
        return;
    }

    Logger::LogInfo("Session ended without a client connection reason=%u", (uint32_t) reason);
    LeaveRoom();
}

void MultiplayerSession::TickHeartbeat()
{
    if (m_heartbeatFuture.valid() && m_heartbeatFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        NetworkResult result = m_heartbeatFuture.get();
        if (result.IsFailure())
        {
            m_heartbeatFailures++;
            Logger::LogError("Session heartbeat failed code=%s failures=%d", result.Format().c_str(), m_heartbeatFailures);
            // Session errors mean the backend dropped or reassigned it, transport errors only end it once the session ttl has surely passed
            bool sessionGone = result.GetGroup() == eErrorGroup_Session && (result.GetCode() == 1 || result.GetCode() == 2);
            if (sessionGone || result.GetGroup() == eErrorGroup_Auth || m_heartbeatFailures >= s_maxHeartbeatFailures)
            {
                EndSession(eDisconnectReason_SessionExpired);
                return;
            }
        }
        else
        {
            m_heartbeatFailures = 0;
        }
    }

    m_heartbeatTimer += (float) Time::GetDelta();
    if (m_heartbeatFuture.valid() || m_heartbeatTimer < s_heartbeatIntervalSeconds)
    {
        return;
    }

    m_heartbeatTimer = 0.0f;

    {
        std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
        m_currentSession.currentPlayers   = EversteadServer::GetInstance()->GetPlayerList()->GetPlayerCount();
        m_currentSession.friendsOfFriends = m_serverLevel->AllowsFriendsOfFriends();
    }

    m_currentSession.joinable = m_currentSession.currentPlayers < m_currentSession.maxPlayers;
    RebuildLocalPlayers();

    m_heartbeatFuture = std::async(std::launch::async, &MultiplayerSession::SendHeartbeat, this, m_currentSession.sid, m_currentSession);
}

void MultiplayerSession::TickPeerHeartbeat()
{
    m_heartbeatTimer += (float) Time::GetDelta();
    if (m_heartbeatTimer < s_heartbeatIntervalSeconds)
    {
        return;
    }

    m_heartbeatTimer = 0.0f;
    PublishSessionAsync(m_currentSession.sid, false);
}

NetworkResult MultiplayerSession::SendHeartbeat(Sid sid, SessionInfo session)
{
    nlohmann::json body;
    body["name"]             = session.name;
    body["visibility"]       = session.visibility;
    body["joinable"]         = session.joinable;
    body["friendsOfFriends"] = session.friendsOfFriends;
    body["maxPlayers"]       = session.maxPlayers;
    body["currentPlayers"]   = session.currentPlayers;

    nlohmann::json document;
    return GetApiClient()->Put("/sessions/" + std::to_string(sid), body, &document);
}

NetworkResult MultiplayerSession::RequestJoinTicket(Sid sid, JoinTicket *outPtr)
{
    nlohmann::json document;
    NetworkResult result = GetApiClient()->Post("/sessions/" + std::to_string(sid) + "/tickets", nlohmann::json::object(), &document);
    if (result.IsFailure())
    {
        return result;
    }

    outPtr->ticket         = document.value("ticket", std::string());
    outPtr->rendezvousHost = document.value("rendezvousHost", std::string());
    outPtr->rendezvousPort = document.value("rendezvousPort", 0);
    outPtr->hostPid        = document.value("hostPid", 0);
    return result;
}

std::vector<MultiplayerSession::SessionInfo> MultiplayerSession::FetchFriendSessions()
{
    std::vector<SessionInfo> sessions;

    nlohmann::json document;
    if (GetApiClient()->Get("/sessions", &document).IsFailure())
    {
        return sessions;
    }

    nlohmann::json entries = document.value("sessions", nlohmann::json::array());
    for (size_t i = 0; i < entries.size(); i++)
    {
        SessionInfo session = ReadSession(entries[i]);
        ReadSessionPlayers(&session);
        sessions.push_back(session);
    }

    return sessions;
}

MultiplayerSession::SessionInfo MultiplayerSession::ReadSession(const nlohmann::json &document)
{
    SessionInfo session;
    session.hostPid          = document.value("hostPid", 0);
    session.hostUsername     = document.value("hostUsername", std::string());
    session.name             = document.value("name", std::string());
    session.visibility       = document.value("visibility", std::string());
    session.joinable         = document.value("joinable", false);
    session.friendsOfFriends = document.value("friendsOfFriends", false);
    session.maxPlayers       = document.value("maxPlayers", 0);
    session.currentPlayers   = document.value("currentPlayers", 0);

    std::string sid = document.value("sid", std::string("0"));
    session.sid     = strtoull(sid.c_str(), nullptr, 10);
    return session;
}

void MultiplayerSession::ReadSessionPlayers(SessionInfo *sessionPtr)
{
    sessionPtr->players.clear();

    Player host;
    host.pid      = sessionPtr->hostPid;
    host.username = sessionPtr->hostUsername;
    host.host     = true;
    host.local    = sessionPtr->hostPid == m_accountService->GetPid();
    sessionPtr->players.push_back(host);

    nlohmann::json document;
    if (GetApiClient()->Get("/sessions/" + std::to_string(sessionPtr->sid) + "/peers", &document).IsFailure())
    {
        return;
    }

    nlohmann::json peers = document.value("peers", nlohmann::json::array());
    for (size_t i = 0; i < peers.size(); i++)
    {
        Player player;
        player.pid      = peers[i].value("pid", 0);
        player.username = peers[i].value("username", std::string());
        player.local    = player.pid == m_accountService->GetPid();
        if (player.pid != sessionPtr->hostPid)
        {
            sessionPtr->players.push_back(player);
        }
    }
}

void MultiplayerSession::ClearCurrentSession()
{
    m_currentSession = SessionInfo();
    m_players.clear();
    m_inSession         = false;
    m_hostingSession    = false;
    m_online            = false;
    m_heartbeatTimer    = 0.0f;
    m_heartbeatFailures = 0;
}

void MultiplayerSession::RebuildLocalPlayers()
{
    m_players.clear();

    Player local;
    local.pid      = GetClientPid();
    local.username = GetClientUsername();
    local.local    = true;
    local.host     = m_hostingSession;
    m_players.push_back(local);

    m_currentSession.players = m_players;
}

ApiClient *MultiplayerSession::GetApiClient() const
{
    return m_accountService->GetApiClient();
}

#endif
