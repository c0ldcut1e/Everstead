#include "ServerConnection.h"

#include "../core/EversteadServer.h"
#include "../core/Logger.h"
#include "Connection.h"
#include "PlayerList.h"
#include "listener/PendingConnection.h"

ServerConnection::ServerConnection(EversteadServer *server)
{
    m_server = server;
}

ServerConnection::~ServerConnection()
{
    Clear();
}

void ServerConnection::AddPendingConnection(PendingConnection *connection)
{
    Logger::LogInfo("Server connection added pending connection connection=%s",
                    connection->GetConnection()->GetName().c_str());
    m_pendingConnections.push_back(std::unique_ptr<PendingConnection>(connection));
}

void ServerConnection::Tick()
{
    for (auto it = m_pendingConnections.begin(); it != m_pendingConnections.end();)
    {
        (*it)->Tick();
        if ((*it)->IsComplete())
        {
            m_server->GetPlayerList()->PlaceNewPlayer((*it)->GetConnection(), (*it)->GetPid(),
                                                      (*it)->GetUsername(), (*it)->GetUniqueID());
            it = m_pendingConnections.erase(it);
        }
        else if ((*it)->GetConnection()->IsClosed())
        {
            Logger::LogInfo("Server connection removed closed pending connection connection=%s",
                            (*it)->GetConnection()->GetName().c_str());
            it = m_pendingConnections.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void ServerConnection::Clear()
{
    m_pendingConnections.clear();
}

bool ServerConnection::HasPendingConnection(Connection *connection) const
{
    for (size_t i = 0; i < m_pendingConnections.size(); i++)
    {
        if (m_pendingConnections[i]->GetConnection() == connection)
        {
            return true;
        }
    }

    return false;
}
