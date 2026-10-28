#include "Connection.h"

#include "../core/Logger.h"
#include "packet/Packet.h"

Connection::Connection(const std::string &name, Socket *socket)
{
    m_name                  = name;
    m_socket                = socket;
    m_byteArrayInputStream  = new ByteArrayInputStream();
    m_byteArrayOutputStream = new ByteArrayOutputStream();
    m_dataInputStream       = new DataInputStream(m_byteArrayInputStream);
    m_dataOutputStream      = new DataOutputStream(m_byteArrayOutputStream);
    m_connected             = true;
    m_flushing              = false;
    m_disconnectReason      = eDisconnectReason_None;
    m_tickCount             = 0;
    m_ping                  = 0;
    Logger::LogInfo("Connection created name=%s", m_name.c_str());
}

Connection::~Connection()
{
    delete m_dataOutputStream;
    delete m_dataInputStream;
    delete m_byteArrayOutputStream;
    delete m_byteArrayInputStream;
    delete m_socket;
}

void Connection::Tick()
{
    m_tickCount++;
    if (m_connected && !m_socket->IsOpen())
    {
        Close(eDisconnectReason_Closed);
    }
}

void Connection::Close(eDISCONNECT_REASON reason)
{
    if (!m_connected.exchange(false))
    {
        return;
    }

    m_disconnectReason = reason;
    m_socket->Close();
    Logger::LogInfo("Connection closed name=%s reason=%u", m_name.c_str(), (uint32_t) reason);
}

// This marks the connection closed but leaves the socket open so the packet channel can write what is still queued
void Connection::CloseAfterFlush(eDISCONNECT_REASON reason)
{
    if (!m_connected.exchange(false))
    {
        return;
    }

    m_disconnectReason = reason;
    m_flushing         = true;
    Logger::LogInfo("Connection closing after flush name=%s reason=%u", m_name.c_str(), (uint32_t) reason);
}

bool Connection::IsFlushing() const
{
    return m_flushing && m_socket->IsOpen();
}

void Connection::Send(std::shared_ptr<Packet> packet)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_sentPackets.push(packet);
}

void Connection::QueueIncomingPacket(std::shared_ptr<Packet> packet)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_receivedPackets.push(packet);
}

void Connection::QueueDelayedPacket(std::shared_ptr<Packet> packet)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_delayedPackets.push(packet);
}

void Connection::QueueIncomingBytes(const std::vector<uint8_t> &bytes, bool serverPacketListener)
{
    std::shared_ptr<Packet> packet = m_packetRegistry.ReadPacket(bytes, serverPacketListener);
    if (!packet)
    {
        Logger::LogError("Connection received unknown packet bytes name=%s size=%zu", m_name.c_str(), bytes.size());
        return;
    }

    QueueIncomingPacket(packet);
}

std::shared_ptr<Packet> Connection::PollSentPacket()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_sentPackets.empty())
    {
        return nullptr;
    }

    std::shared_ptr<Packet> packet = m_sentPackets.front();
    m_sentPackets.pop();
    return packet;
}

bool Connection::PollSentBytes(std::vector<uint8_t> *outPtr)
{
    std::shared_ptr<Packet> packet = PollSentPacket();
    if (!packet)
    {
        return false;
    }

    return m_packetRegistry.WritePacket(packet.get(), outPtr);
}

std::shared_ptr<Packet> Connection::PollReceivedPacket(bool includeDelayed)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::queue<std::shared_ptr<Packet>> *queue = &m_receivedPackets;
    if (queue->empty() && includeDelayed)
    {
        queue = &m_delayedPackets;
    }

    if (queue->empty())
    {
        return nullptr;
    }

    std::shared_ptr<Packet> packet = queue->front();
    queue->pop();
    return packet;
}

size_t Connection::GetSentPacketCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_sentPackets.size();
}

size_t Connection::GetReceivedPacketCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_receivedPackets.size();
}

size_t Connection::GetDelayedPacketCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_delayedPackets.size();
}

bool Connection::IsConnected() const
{
    return m_connected && m_socket->IsOpen();
}

bool Connection::IsClosed() const
{
    return !IsConnected();
}

eDISCONNECT_REASON Connection::GetDisconnectReason() const
{
    return m_disconnectReason;
}

void Connection::SetName(const std::string &name)
{
    m_name = name;
}

const std::string &Connection::GetName() const
{
    return m_name;
}

void Connection::SetPing(int ping)
{
    m_ping = ping;
}

int Connection::GetPing() const
{
    return m_ping;
}

Socket *Connection::GetSocket() const
{
    return m_socket;
}

DataInputStream *Connection::GetDataInputStream() const
{
    return m_dataInputStream;
}

DataOutputStream *Connection::GetDataOutputStream() const
{
    return m_dataOutputStream;
}

ByteArrayInputStream *Connection::GetByteArrayInputStream() const
{
    return m_byteArrayInputStream;
}

ByteArrayOutputStream *Connection::GetByteArrayOutputStream() const
{
    return m_byteArrayOutputStream;
}
