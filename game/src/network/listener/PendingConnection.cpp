#include "PendingConnection.h"

#include <cwchar>

#include "../../core/GameInfo.h"
#include "../../core/Logger.h"
#include "../Connection.h"
#include "../packet/Packet.h"
#include "../packet/c2s/LoginC2SPacket.h"
#include "../packet/c2s/PreLoginC2SPacket.h"
#include "../packet/s2c/DisconnectS2CPacket.h"

PendingConnection::PendingConnection(Connection *connection, Sid sid, Pid pid)
{
    m_connection       = connection;
    m_sid              = sid;
    m_pid              = pid;
    m_uniqueID         = {};
    m_username         = "";
    m_tick             = 0;
    m_receivedPreLogin = false;
    m_complete         = false;
    m_disconnected     = false;
    Logger::LogInfo("Pending connection created connection=%s sid=%llu", m_connection->GetName().c_str(), (unsigned long long) m_sid);
}

PendingConnection::~PendingConnection()
{}

void PendingConnection::Tick()
{
    if (m_complete || m_disconnected)
    {
        return;
    }

    m_tick++;
    if (m_tick > s_maxTicksBeforeLogin)
    {
        Disconnect(eDisconnectReason_LoginTooLong);
        return;
    }

    std::shared_ptr<Packet> packet = m_connection->PollSentPacket();
    while (packet && !m_disconnected)
    {
        packet->Handle(this);
        packet = m_connection->PollSentPacket();
    }
}

void PendingConnection::Disconnect(eDISCONNECT_REASON reason)
{
    if (m_disconnected)
    {
        return;
    }

    m_disconnected = true;
    Logger::LogInfo("Pending connection rejected connection=%s pid=%u reason=%s", m_connection->GetName().c_str(), m_pid,
                    DisconnectS2CPacket::GetReasonKey(reason).c_str());
    wchar_t version[32];
    swprintf(version, 32, L"%u.%u.%u", g_gameVersionMajor, g_gameVersionMinor, g_gameVersionPatch);
    Send(std::shared_ptr<Packet>(new DisconnectS2CPacket(reason, version)));
    m_connection->CloseAfterFlush(reason);
}

void PendingConnection::Send(std::shared_ptr<Packet> packet)
{
    m_connection->QueueIncomingPacket(packet);
}

bool PendingConnection::IsServerPacketListener()
{
    return true;
}

void PendingConnection::OnUnhandledPacket(Packet *packet)
{
    Logger::LogInfo("Pending connection unexpected packet connection=%s packetId=%d", m_connection->GetName().c_str(), packet->GetId());
    Disconnect(eDisconnectReason_UnexpectedPacket);
}

void PendingConnection::HandlePreLogin(PreLoginC2SPacket *packet)
{
    if (packet->GetVersionMajor() < g_gameVersionMajor ||
        (packet->GetVersionMajor() == g_gameVersionMajor && packet->GetVersionMinor() < g_gameVersionMinor) ||
        (packet->GetVersionMajor() == g_gameVersionMajor && packet->GetVersionMinor() == g_gameVersionMinor &&
         packet->GetVersionPatch() < g_gameVersionPatch))
    {
        Disconnect(eDisconnectReason_OlderVersion);
        return;
    }

    if (packet->GetVersionMajor() > g_gameVersionMajor ||
        (packet->GetVersionMajor() == g_gameVersionMajor && packet->GetVersionMinor() > g_gameVersionMinor) ||
        (packet->GetVersionMajor() == g_gameVersionMajor && packet->GetVersionMinor() == g_gameVersionMinor &&
         packet->GetVersionPatch() > g_gameVersionPatch))
    {
        Disconnect(eDisconnectReason_NewerVersion);
        return;
    }

    if (packet->GetPid() != m_pid)
    {
        Logger::LogInfo("Pending connection pid mismatch connection=%s expected=%u received=%u", m_connection->GetName().c_str(), m_pid,
                        packet->GetPid());
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    m_username         = packet->GetUsername();
    m_receivedPreLogin = true;
    Logger::LogInfo("Pending connection pre-login connection=%s pid=%u username=%s", m_connection->GetName().c_str(), m_pid, m_username.c_str());
}

void PendingConnection::HandleLogin(LoginC2SPacket *packet)
{
    if (!m_receivedPreLogin)
    {
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    if (packet->GetPid() != m_pid)
    {
        Logger::LogInfo("Pending connection pid mismatch connection=%s expected=%u received=%u", m_connection->GetName().c_str(), m_pid,
                        packet->GetPid());
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    m_complete = true;
    m_uniqueID = packet->GetUniqueID();
    Logger::LogInfo("Pending connection login complete connection=%s pid=%u username=%s uid=%s", m_connection->GetName().c_str(), m_pid,
                    m_username.c_str(), m_uniqueID.ToString().c_str());
}

bool PendingConnection::IsComplete() const
{
    return m_complete;
}

Pid PendingConnection::GetPid() const
{
    return m_pid;
}

const UniqueID &PendingConnection::GetUniqueID() const
{
    return m_uniqueID;
}

const std::string &PendingConnection::GetUsername() const
{
    return m_username;
}

Connection *PendingConnection::GetConnection() const
{
    return m_connection;
}
