#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../network/ApiClient.h"
#include "../network/NetworkTypes.h"

class Connection;
class PeerNetwork;
struct ServerConfig;

class ServerSession
{
public:
    explicit ServerSession(const ServerConfig &config);
    ~ServerSession();

    bool Start(int maxPlayers);
    void Stop();
    void Tick(int playerCount);

    bool HasExpired() const;
    Sid GetSid() const;
    const std::string &GetName() const;

private:
    nlohmann::json MakeSessionBody() const;
    void RunHeartbeat();
    void AcceptConnections();
    void PruneConnections();

    const ServerConfig &m_config;
    ApiClient *m_apiClient;
    PeerNetwork *m_peerNetwork;
    std::vector<Connection *> m_connections;
    Sid m_sid;
    std::string m_name;
    int m_maxPlayers;
    std::atomic<int> m_playerCount;
    std::atomic<bool> m_expired;
    bool m_running;
    std::mutex m_heartbeatMutex;
    std::condition_variable m_heartbeatCondition;
    std::thread m_heartbeatThread;
};
