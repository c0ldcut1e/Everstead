#pragma once

#include <memory>
#include <vector>

#include "../core/EversteadServer.h"
#include "listener/PendingConnection.h"

class ServerConnection
{
public:
    explicit ServerConnection(EversteadServer *server);
    ~ServerConnection();

    void AddPendingConnection(PendingConnection *connection);
    void Tick();
    void Clear();
    bool HasPendingConnection(Connection *connection) const;

private:
    EversteadServer *m_server;
    std::vector<std::unique_ptr<PendingConnection>> m_pendingConnections;
};
