#pragma once

#include <memory>
#include <string>

#include "../../utils/UniqueID.h"
#include "../Connection.h"
#include "../NetworkTypes.h"
#include "../packet/Packet.h"
#include "PacketListener.h"

class PendingConnection : public PacketListener
{
public:
    PendingConnection(Connection *connection, Sid sid, Pid pid);
    ~PendingConnection() override;

    void Tick();
    void Disconnect(eDISCONNECT_REASON reason);
    void Send(std::shared_ptr<Packet> packet);

    bool IsServerPacketListener() override;
    void OnUnhandledPacket(Packet *packet) override;
    void HandlePreLogin(PreLoginC2SPacket *packet) override;
    void HandleLogin(LoginC2SPacket *packet) override;

    bool IsComplete() const;
    Pid GetPid() const;
    const UniqueID &GetUniqueID() const;
    const std::string &GetUsername() const;
    Connection *GetConnection() const;

private:
    static const int s_maxTicksBeforeLogin = 20 * 30;

    Connection *m_connection;
    Sid m_sid;
    Pid m_pid;
    UniqueID m_uniqueID;
    std::string m_username;
    int m_tick;
    bool m_receivedPreLogin;
    bool m_complete;
    bool m_disconnected;
};
