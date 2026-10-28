#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "../io/ByteArrayInputStream.h"
#include "../io/ByteArrayOutputStream.h"
#include "../io/DataInputStream.h"
#include "../io/DataOutputStream.h"
#include "Socket.h"
#include "packet/PacketRegistry.h"
#include "packet/s2c/DisconnectS2CPacket.h"

class Connection
{
public:
    Connection(const std::string &name, Socket *socket);
    ~Connection();

    void Tick();
    void Close(eDISCONNECT_REASON reason);
    void CloseAfterFlush(eDISCONNECT_REASON reason);
    bool IsFlushing() const;

    void Send(std::shared_ptr<Packet> packet);
    void QueueIncomingPacket(std::shared_ptr<Packet> packet);
    void QueueDelayedPacket(std::shared_ptr<Packet> packet);
    void QueueIncomingBytes(const std::vector<uint8_t> &bytes, bool serverPacketListener);
    std::shared_ptr<Packet> PollSentPacket();
    bool PollSentBytes(std::vector<uint8_t> *outPtr);
    std::shared_ptr<Packet> PollReceivedPacket(bool includeDelayed);
    size_t GetSentPacketCount() const;
    size_t GetReceivedPacketCount() const;
    size_t GetDelayedPacketCount() const;

    bool IsConnected() const;
    bool IsClosed() const;
    eDISCONNECT_REASON GetDisconnectReason() const;

    void SetName(const std::string &name);
    const std::string &GetName() const;

    void SetPing(int ping);
    int GetPing() const;

    Socket *GetSocket() const;
    DataInputStream *GetDataInputStream() const;
    DataOutputStream *GetDataOutputStream() const;
    ByteArrayInputStream *GetByteArrayInputStream() const;
    ByteArrayOutputStream *GetByteArrayOutputStream() const;

private:
    std::string m_name;
    Socket *m_socket;
    ByteArrayInputStream *m_byteArrayInputStream;
    ByteArrayOutputStream *m_byteArrayOutputStream;
    DataInputStream *m_dataInputStream;
    DataOutputStream *m_dataOutputStream;
    std::queue<std::shared_ptr<Packet>> m_sentPackets;
    std::queue<std::shared_ptr<Packet>> m_receivedPackets;
    std::queue<std::shared_ptr<Packet>> m_delayedPackets;
    PacketRegistry m_packetRegistry;
    mutable std::mutex m_queueMutex;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_flushing;
    std::atomic<eDISCONNECT_REASON> m_disconnectReason;
    int m_tickCount;
    std::atomic<int> m_ping;
};
