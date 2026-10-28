#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Connection.h"
#include "NetworkTypes.h"
#include "packet/PacketRegistry.h"

typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;
typedef struct _ENetEvent ENetEvent;

class PeerNetwork
{
public:
    PeerNetwork();
    ~PeerNetwork();

    PeerNetwork(const PeerNetwork &)            = delete;
    PeerNetwork &operator=(const PeerNetwork &) = delete;

    bool StartHost(const std::string &rendezvousHost, int rendezvousPort, const std::string &hostKey, int localPort);
    bool StartJoin(const std::string &rendezvousHost, int rendezvousPort, const std::string &ticket);

    bool PollAcceptedConnection(Connection **outPtr, Pid *pidOutPtr);
    void RemoveConnection(Connection *connection);
    bool HasFailed() const;

private:
    typedef std::chrono::steady_clock Clock;

    struct Address
    {
        uint32_t host;
        uint16_t port;
    };

    struct Link
    {
        ENetPeer *peer;
        Connection *connection;
        Pid pid;
        bool flushStarted;
        Clock::time_point flushStart;
    };

    struct Contact
    {
        uint64_t token;
        Pid pid;
        std::vector<Address> addresses;
        Clock::time_point expiry;
        Clock::time_point nextProbe;
    };

    struct AllowedAddress
    {
        Address address;
        Pid pid;
        Clock::time_point expiry;
    };

    struct AcceptedConnection
    {
        Connection *connection;
        Pid pid;
    };

    enum eKIND
    {
        eKind_HostRegister   = 1,
        eKind_HostRegistered = 2,
        eKind_JoinRegister   = 3,
        eKind_Introduce      = 4,
        eKind_HostInfo       = 5,
        eKind_JoinRefused    = 6,
        eKind_Probe          = 7,
        eKind_ProbeAck       = 8,
    };

    static const uint32_t s_magic               = 0x45565244;
    static const uint8_t s_protocolVersion      = 1;
    static const size_t s_headerSize            = 6;
    static const size_t s_maxCandidates         = 8;
    static const size_t s_maxPeers              = 64;
    static const size_t s_compressThreshold     = 256;
    static const size_t s_maxPacketSize         = 4 * 1024 * 1024;
    static const size_t s_maxPulledBytes        = 16 * 1024;
    static inline const int s_flushTimeoutMs    = 2000;
    static inline const int s_contactLifetimeMs = 30000;
    static inline const int s_probeIntervalMs   = 250;
    static inline const int s_joinTimeoutMs     = 20000;

    static thread_local PeerNetwork *s_current;

    static int Intercept(ENetHost *host, ENetEvent *event);

    bool Start(const std::string &rendezvousHost, int rendezvousPort, const std::string &secret, bool hosting, int localPort);
    void Run();
    void TickRendezvous(Clock::time_point now);
    void SendRegister();
    void SendProbes(Contact *contact, Clock::time_point now);
    void SendRaw(const Address &address, const std::vector<uint8_t> &bytes);
    std::vector<uint8_t> MakeHeader(uint8_t kind);
    bool HandleRaw(const Address &source, const uint8_t *data, size_t size);
    void HandleRendezvous(uint8_t kind, const uint8_t *body, size_t size);
    void HandleProbe(const Address &source, uint8_t kind, uint64_t token);
    bool IsAllowed(const Address &address) const;
    void HandleEvent(const ENetEvent &event);
    void PumpLink(Link *link);
    void TickFlush(Link *link);
    void SendPayload(Link *link, const std::vector<uint8_t> &bytes);
    bool DecodePayload(const uint8_t *data, size_t size, std::vector<uint8_t> *outPtr);

    ENetHost *m_host;
    Address m_rendezvous;
    std::string m_secret;
    bool m_hosting;
    bool m_registered;
    bool m_hostOffline;
    Clock::time_point m_nextRegister;
    Clock::time_point m_joinDeadline;
    Clock::time_point m_nextStatsLog;
    ENetPeer *m_hostPeer;
    std::vector<uint8_t> m_candidates;
    std::vector<Contact> m_contacts;
    std::vector<AllowedAddress> m_allowed;
    std::vector<Link *> m_links;
    std::vector<AcceptedConnection> m_accepted;
    PacketRegistry m_packetRegistry;
    std::mutex m_mutex;
    std::atomic<bool> m_running;
    std::atomic<bool> m_failed;
    std::thread m_thread;
};
