#ifdef EVERSTEAD_DEDICATED_SERVER

#include "ServerSession.h"

#include <chrono>
#include <cstdlib>

#include "../core/EversteadServer.h"
#include "../core/Logger.h"
#include "../network/Connection.h"
#include "../network/PeerNetwork.h"
#include "../network/PlayerList.h"
#include "../network/ServerConnection.h"
#include "../network/listener/PendingConnection.h"
#include "../network/packet/s2c/DisconnectS2CPacket.h"
#include "ServerConfig.h"

ServerSession::ServerSession(const ServerConfig &config) : m_config(config)
{
    if (!config.backendServer.empty())
    {
        ApiClient::SetServerOverride(config.backendServer);
    }

    m_apiClient = new ApiClient();
    m_apiClient->SetAccessToken(config.token);
    m_peerNetwork = nullptr;
    m_sid         = 0;
    m_name        = "";
    m_maxPlayers  = 0;
    m_playerCount = 0;
    m_expired     = false;
    m_running     = false;
}

ServerSession::~ServerSession()
{
    Stop();
    delete m_apiClient;
}

bool ServerSession::Start(int maxPlayers)
{
    m_maxPlayers = maxPlayers;
    if (m_config.token.empty())
    {
        Logger::LogError("The server needs session.token in its config, create a server on the account page to get one");
        return false;
    }

    nlohmann::json document;
    NetworkResult result = m_apiClient->Post("/server/sessions", MakeSessionBody(), &document);
    if (result.IsFailure())
    {
        Logger::LogError("Could not create the session code=%s", result.Format().c_str());
        return false;
    }

    m_sid         = strtoull(document.value("sid", std::string("0")).c_str(), nullptr, 10);
    m_name        = document.value("name", std::string());
    m_peerNetwork = new PeerNetwork();
    if (!m_peerNetwork->StartHost(document.value("rendezvousHost", std::string()), document.value("rendezvousPort", 0),
                                  document.value("hostKey", std::string()), m_config.port))
    {
        Logger::LogError("Could not reach the rendezvous server");
        delete m_peerNetwork;
        m_peerNetwork = nullptr;
        m_apiClient->Delete("/server/sessions/" + std::to_string(m_sid));
        return false;
    }

    m_running         = true;
    m_heartbeatThread = std::thread(&ServerSession::RunHeartbeat, this);
    Logger::LogInfo("Hosting session sid=%llu name=%s", (unsigned long long) m_sid, m_name.c_str());
    return true;
}

void ServerSession::Stop()
{
    if (!m_peerNetwork)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_heartbeatMutex);
        m_running = false;
    }

    m_heartbeatCondition.notify_all();
    m_heartbeatThread.join();

    for (size_t i = 0; i < m_connections.size(); i++)
    {
        m_connections[i]->QueueIncomingPacket(std::shared_ptr<Packet>(new DisconnectS2CPacket(eDisconnectReason_ServerStopping)));
        m_connections[i]->CloseAfterFlush(eDisconnectReason_ServerStopping);
    }

    // We give the network thread a moment to write the disconnect so players see why they left
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline)
    {
        bool open = false;
        for (size_t i = 0; i < m_connections.size(); i++)
        {
            open = open || m_connections[i]->GetSocket()->IsOpen();
        }

        if (!open)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    m_apiClient->Delete("/server/sessions/" + std::to_string(m_sid));

    delete m_peerNetwork;
    m_peerNetwork = nullptr;
    for (size_t i = 0; i < m_connections.size(); i++)
    {
        delete m_connections[i];
    }

    m_connections.clear();
}

void ServerSession::Tick(int playerCount)
{
    m_playerCount = playerCount;
    AcceptConnections();
    PruneConnections();
}

bool ServerSession::HasExpired() const
{
    return m_expired || (m_peerNetwork && m_peerNetwork->HasFailed());
}

Sid ServerSession::GetSid() const
{
    return m_sid;
}

const std::string &ServerSession::GetName() const
{
    return m_name;
}

nlohmann::json ServerSession::MakeSessionBody() const
{
    nlohmann::json body;
    body["maxPlayers"]     = m_maxPlayers;
    body["currentPlayers"] = (int) m_playerCount;
    return body;
}

void ServerSession::RunHeartbeat()
{
    int failures = 0;
    std::unique_lock<std::mutex> lock(m_heartbeatMutex);
    while (m_running)
    {
        m_heartbeatCondition.wait_for(lock, std::chrono::seconds(30));
        if (!m_running)
        {
            return;
        }

        lock.unlock();
        nlohmann::json document;
        NetworkResult result = m_apiClient->Put("/server/sessions/" + std::to_string(m_sid), MakeSessionBody(), &document);
        lock.lock();
        if (result.IsSuccess())
        {
            failures = 0;
            continue;
        }

        failures++;
        Logger::LogError("Session heartbeat failed code=%s failures=%d", result.Format().c_str(), failures);
        bool sessionGone = result.GetGroup() == eErrorGroup_Session && result.GetCode() == 1;
        if (sessionGone || result.GetGroup() == eErrorGroup_Server || failures >= 4)
        {
            m_expired = true;
            return;
        }
    }
}

void ServerSession::AcceptConnections()
{
    Connection *connection = nullptr;
    Pid pid                = 0;
    while (m_peerNetwork->PollAcceptedConnection(&connection, &pid))
    {
        EversteadServer::GetInstance()->GetConnection()->AddPendingConnection(new PendingConnection(connection, m_sid, pid));
        m_connections.push_back(connection);
        Logger::LogInfo("Accepted a player connection name=%s pid=%u", connection->GetName().c_str(), pid);
    }
}

void ServerSession::PruneConnections()
{
    size_t i = 0;
    while (i < m_connections.size())
    {
        Connection *connection = m_connections[i];
        if (!connection->IsClosed() || connection->IsFlushing() || connection->GetReceivedPacketCount() > 0 ||
            EversteadServer::GetInstance()->GetConnection()->HasPendingConnection(connection) ||
            EversteadServer::GetInstance()->GetPlayerList()->HasConnection(connection))
        {
            i++;
            continue;
        }

        m_peerNetwork->RemoveConnection(connection);
        delete connection;
        m_connections.erase(m_connections.begin() + (long) i);
    }
}

#endif
