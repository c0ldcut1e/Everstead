#include "PeerNetwork.h"

#include <algorithm>
#include <cstring>

#include "../core/Logger.h"
#include "../platform/PlatformNet.h"
#include "packet/Packet.h"

#include <enet/enet.h>
#include <zlib.h>

thread_local PeerNetwork *PeerNetwork::s_current = nullptr;

PeerNetwork::PeerNetwork()
{
    m_host         = nullptr;
    m_rendezvous   = {};
    m_secret       = "";
    m_hosting      = false;
    m_registered   = false;
    m_hostOffline  = false;
    m_nextRegister = {};
    m_joinDeadline = {};
    m_nextStatsLog = {};
    m_hostPeer     = nullptr;
    m_candidates   = {};
    m_contacts     = {};
    m_allowed      = {};
    m_links        = {};
    m_accepted     = {};
    m_running      = false;
    m_failed       = false;
}

PeerNetwork::~PeerNetwork()
{
    m_running = false;
    if (m_thread.joinable())
    {
        m_thread.join();
    }

    for (size_t i = 0; i < m_links.size(); i++)
    {
        if (m_links[i]->connection)
        {
            m_links[i]->connection->Close(eDisconnectReason_Closed);
        }

        enet_peer_disconnect_now(m_links[i]->peer, 0);
        delete m_links[i];
    }

    for (size_t i = 0; i < m_accepted.size(); i++)
    {
        delete m_accepted[i].connection;
    }

    if (m_host)
    {
        enet_host_destroy(m_host);
    }
}

bool PeerNetwork::StartHost(const std::string &rendezvousHost, int rendezvousPort, const std::string &hostKey, int localPort)
{
    return Start(rendezvousHost, rendezvousPort, hostKey, true, localPort);
}

bool PeerNetwork::StartJoin(const std::string &rendezvousHost, int rendezvousPort, const std::string &ticket)
{
    return Start(rendezvousHost, rendezvousPort, ticket, false, 0);
}

bool PeerNetwork::Start(const std::string &rendezvousHost, int rendezvousPort, const std::string &secret, bool hosting, int localPort)
{
    static const bool s_enetReady = enet_initialize() == 0;
    ENetAddress rendezvous;
    if (!s_enetReady || secret.empty() || secret.size() > 255 || enet_address_set_host(&rendezvous, rendezvousHost.c_str()) != 0)
    {
        Logger::LogError("Peer network could not resolve the rendezvous host=%s", rendezvousHost.c_str());
        return false;
    }

    m_rendezvous = {rendezvous.host, (uint16_t) rendezvousPort};

    ENetAddress bindAddress;
    bindAddress.host = ENET_HOST_ANY;
    bindAddress.port = (uint16_t) localPort;
    m_host           = enet_host_create(&bindAddress, hosting ? s_maxPeers : 1, 1, 0, 0);
    if (!m_host)
    {
        Logger::LogError("Peer network could not open a UDP socket port=%d", localPort);
        return false;
    }

    PlatformNet::DisableConnectionReset((PlatformNet::Handle) m_host->socket);
    m_host->intercept = &PeerNetwork::Intercept;
    // Candidates can route through VPN adapters with a small MTU that silently drop full size fragments
    m_host->mtu = 1200;

    std::vector<uint32_t> localAddresses = PlatformNet::GetLocalIpv4Addresses();
    size_t count                         = std::min(localAddresses.size(), (size_t) s_maxCandidates);
    m_candidates.push_back((uint8_t) count);
    for (size_t i = 0; i < count; i++)
    {
        const uint8_t *ip = (const uint8_t *) &localAddresses[i];
        m_candidates.insert(m_candidates.end(), ip, ip + 4);
        m_candidates.push_back((uint8_t) (m_host->address.port >> 8));
        m_candidates.push_back((uint8_t) (m_host->address.port & 0xFF));
    }

    m_secret       = secret;
    m_hosting      = hosting;
    m_nextRegister = Clock::now();
    m_joinDeadline = m_nextRegister + std::chrono::milliseconds(s_joinTimeoutMs);
    m_running      = true;
    m_thread       = std::thread(&PeerNetwork::Run, this);

    Logger::LogInfo("Peer network started hosting=%d port=%u candidates=%zu", hosting, m_host->address.port, count);
    return true;
}

bool PeerNetwork::PollAcceptedConnection(Connection **outPtr, Pid *pidOutPtr)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_accepted.empty())
    {
        return false;
    }

    *outPtr    = m_accepted.front().connection;
    *pidOutPtr = m_accepted.front().pid;
    m_accepted.erase(m_accepted.begin());
    return true;
}

void PeerNetwork::RemoveConnection(Connection *connection)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_links.size(); i++)
    {
        if (m_links[i]->connection == connection)
        {
            m_links[i]->connection = nullptr;
            enet_peer_disconnect_later(m_links[i]->peer, 0);
        }
    }
}

bool PeerNetwork::HasFailed() const
{
    return m_failed;
}

int PeerNetwork::Intercept(ENetHost *host, ENetEvent *event)
{
    (void) event;
    return s_current->HandleRaw({host->receivedAddress.host, host->receivedAddress.port}, host->receivedData, host->receivedDataLength) ? 1 : 0;
}

void PeerNetwork::Run()
{
    s_current = this;
    while (m_running)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            Clock::time_point now = Clock::now();
            TickRendezvous(now);
            for (size_t i = 0; i < m_links.size(); i++)
            {
                PumpLink(m_links[i]);
            }

            if (now >= m_nextStatsLog)
            {
                m_nextStatsLog = now + std::chrono::seconds(5);
                for (size_t i = 0; i < m_links.size(); i++)
                {
                    ENetPeer *peer         = m_links[i]->peer;
                    Connection *connection = m_links[i]->connection;
                    Logger::LogInfo("Peer network stats pid=%u rtt=%u rttVar=%u throttle=%u inTransit=%u window=%u lost=%u queued=%zu delayed=%zu",
                                    m_links[i]->pid, peer->roundTripTime, peer->roundTripTimeVariance, peer->packetThrottle,
                                    peer->reliableDataInTransit, peer->windowSize, peer->packetsLost,
                                    m_hosting ? connection->GetReceivedPacketCount() : connection->GetSentPacketCount(),
                                    m_hosting ? connection->GetDelayedPacketCount() : (size_t) 0);
                }
            }

            ENetEvent event;
            while (enet_host_service(m_host, &event, 0) > 0)
            {
                HandleEvent(event);
            }
        }

        enet_uint32 condition = ENET_SOCKET_WAIT_RECEIVE | ENET_SOCKET_WAIT_INTERRUPT;
        enet_socket_wait(m_host->socket, &condition, 1);
    }
}

void PeerNetwork::TickRendezvous(Clock::time_point now)
{
    if (!m_hosting && !m_hostPeer && !m_failed && now >= m_joinDeadline)
    {
        Logger::LogError("Peer network could not reach the host before the join timeout contacts=%zu hostOffline=%d", m_contacts.size(),
                         m_hostOffline);
        for (size_t i = 0; i < m_contacts.size(); i++)
        {
            for (size_t j = 0; j < m_contacts[i].addresses.size(); j++)
            {
                const uint8_t *ip = (const uint8_t *) &m_contacts[i].addresses[j].host;
                Logger::LogError("Peer network probed %u.%u.%u.%u:%u without an answer", ip[0], ip[1], ip[2], ip[3], m_contacts[i].addresses[j].port);
            }
        }

        m_failed = true;
    }

    bool joining = !m_hosting && !m_failed && (!m_hostPeer || m_hostPeer->state != ENET_PEER_STATE_CONNECTED);
    if ((m_hosting || joining) && now >= m_nextRegister)
    {
        SendRegister();
        int intervalMs = 1000;
        if (m_hosting && m_registered)
        {
            intervalMs = 10000;
        }
        else if (!m_hosting && m_contacts.empty())
        {
            intervalMs = 500;
        }

        m_nextRegister = now + std::chrono::milliseconds(intervalMs);
    }

    size_t i = 0;
    while (i < m_contacts.size())
    {
        if (now >= m_contacts[i].expiry)
        {
            m_contacts.erase(m_contacts.begin() + (long) i);
            continue;
        }

        if (joining || m_hosting)
        {
            SendProbes(&m_contacts[i], now);
        }

        i++;
    }

    i = 0;
    while (i < m_allowed.size())
    {
        bool linked = false;
        for (size_t j = 0; j < m_links.size(); j++)
        {
            ENetAddress address = m_links[j]->peer->address;
            if (address.host == m_allowed[i].address.host && address.port == m_allowed[i].address.port)
            {
                linked = true;
                break;
            }
        }

        if (!linked && now >= m_allowed[i].expiry)
        {
            m_allowed.erase(m_allowed.begin() + (long) i);
            continue;
        }

        i++;
    }
}

void PeerNetwork::SendRegister()
{
    std::vector<uint8_t> bytes = MakeHeader(m_hosting ? eKind_HostRegister : eKind_JoinRegister);
    bytes.push_back((uint8_t) m_secret.size());
    bytes.insert(bytes.end(), m_secret.begin(), m_secret.end());
    bytes.insert(bytes.end(), m_candidates.begin(), m_candidates.end());
    SendRaw(m_rendezvous, bytes);
}

void PeerNetwork::SendProbes(Contact *contact, Clock::time_point now)
{
    if (now < contact->nextProbe)
    {
        return;
    }

    contact->nextProbe         = now + std::chrono::milliseconds(s_probeIntervalMs);
    std::vector<uint8_t> bytes = MakeHeader(eKind_Probe);
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        bytes.push_back((uint8_t) (contact->token >> shift));
    }

    for (size_t i = 0; i < contact->addresses.size(); i++)
    {
        SendRaw(contact->addresses[i], bytes);
    }
}

void PeerNetwork::SendRaw(const Address &address, const std::vector<uint8_t> &bytes)
{
    ENetAddress target = {address.host, address.port};
    ENetBuffer buffer;
    buffer.data       = (void *) bytes.data();
    buffer.dataLength = bytes.size();
    enet_socket_send(m_host->socket, &target, &buffer, 1);
}

std::vector<uint8_t> PeerNetwork::MakeHeader(uint8_t kind)
{
    return {(uint8_t) (s_magic >> 24), (uint8_t) (s_magic >> 16), (uint8_t) (s_magic >> 8), (uint8_t) s_magic, s_protocolVersion, kind};
}

bool PeerNetwork::HandleRaw(const Address &source, const uint8_t *data, size_t size)
{
    uint32_t magic = size >= s_headerSize ? ((uint32_t) data[0] << 24) | ((uint32_t) data[1] << 16) | ((uint32_t) data[2] << 8) | data[3] : 0;
    if (magic != s_magic || data[4] != s_protocolVersion)
    {
        return !IsAllowed(source);
    }

    uint8_t kind = data[5];
    if (source.host == m_rendezvous.host && source.port == m_rendezvous.port)
    {
        HandleRendezvous(kind, data + s_headerSize, size - s_headerSize);
    }
    else if ((kind == eKind_Probe || kind == eKind_ProbeAck) && size == s_headerSize + 8)
    {
        uint64_t token = 0;
        for (size_t i = 0; i < 8; i++)
        {
            token = (token << 8) | data[s_headerSize + i];
        }

        HandleProbe(source, kind, token);
    }

    return true;
}

void PeerNetwork::HandleRendezvous(uint8_t kind, const uint8_t *body, size_t size)
{
    if (kind == eKind_HostRegistered && m_hosting)
    {
        m_registered = true;
        return;
    }

    if (kind == eKind_JoinRefused && !m_hosting && size >= 1)
    {
        // The host may only be between registrations, so only a rejected ticket ends the join early
        if (body[0] == 1)
        {
            Logger::LogError("Peer network join ticket was refused");
            m_failed = true;
        }
        else
        {
            m_hostOffline = true;
        }

        return;
    }

    size_t offset = 0;
    Contact contact;
    contact.pid = 0;
    if (kind == eKind_Introduce && m_hosting && size >= 4)
    {
        contact.pid = ((Pid) body[0] << 24) | ((Pid) body[1] << 16) | ((Pid) body[2] << 8) | body[3];
        offset      = 4;
    }
    else if (kind != eKind_HostInfo || m_hosting)
    {
        return;
    }

    if (size < offset + 8 + 6 + 1)
    {
        return;
    }

    contact.token = 0;
    for (size_t i = 0; i < 8; i++)
    {
        contact.token = (contact.token << 8) | body[offset + i];
    }

    offset += 8;
    size_t count = body[offset + 6];
    if (count > s_maxCandidates || size != offset + 7 + count * 6)
    {
        return;
    }

    for (size_t i = 0; i <= count; i++)
    {
        const uint8_t *entry = i == 0 ? body + offset : body + offset + 7 + (i - 1) * 6;
        Address address;
        memcpy(&address.host, entry, 4);
        address.port = (uint16_t) ((entry[4] << 8) | entry[5]);
        contact.addresses.push_back(address);
    }

    for (size_t i = 0; i < m_contacts.size(); i++)
    {
        if (m_contacts[i].token == contact.token)
        {
            return;
        }
    }

    contact.expiry    = Clock::now() + std::chrono::milliseconds(s_contactLifetimeMs);
    contact.nextProbe = Clock::now();
    m_contacts.push_back(contact);
    Logger::LogInfo("Peer network introduced pid=%u addresses=%zu", contact.pid, contact.addresses.size());
}

void PeerNetwork::HandleProbe(const Address &source, uint8_t kind, uint64_t token)
{
    const Contact *contact = nullptr;
    for (size_t i = 0; i < m_contacts.size(); i++)
    {
        if (m_contacts[i].token == token)
        {
            contact = &m_contacts[i];
            break;
        }
    }

    if (!contact)
    {
        return;
    }

    if (kind == eKind_Probe)
    {
        std::vector<uint8_t> bytes = MakeHeader(eKind_ProbeAck);
        for (int shift = 56; shift >= 0; shift -= 8)
        {
            bytes.push_back((uint8_t) (token >> shift));
        }

        SendRaw(source, bytes);
    }

    if (IsAllowed(source))
    {
        return;
    }

    AllowedAddress allowed;
    allowed.address = source;
    allowed.pid     = contact->pid;
    allowed.expiry  = contact->expiry;
    m_allowed.push_back(allowed);

    if (!m_hosting && !m_hostPeer)
    {
        ENetAddress target = {source.host, source.port};
        m_hostPeer         = enet_host_connect(m_host, &target, 1, 0);
        Logger::LogInfo("Peer network reached the host port=%u", source.port);
    }
}

bool PeerNetwork::IsAllowed(const Address &address) const
{
    for (size_t i = 0; i < m_allowed.size(); i++)
    {
        if (m_allowed[i].address.host == address.host && m_allowed[i].address.port == address.port)
        {
            return true;
        }
    }

    return false;
}

void PeerNetwork::HandleEvent(const ENetEvent &event)
{
    if (event.type == ENET_EVENT_TYPE_CONNECT)
    {
        Pid pid = 0;
        for (size_t i = 0; i < m_allowed.size(); i++)
        {
            if (m_allowed[i].address.host == event.peer->address.host && m_allowed[i].address.port == event.peer->address.port)
            {
                pid = m_allowed[i].pid;
                break;
            }
        }

        std::string name = (m_hosting ? "peer-" : "host-") + std::to_string(pid);
        Link *link       = new Link();
        link->peer       = event.peer;
        link->connection = new Connection(name, new Socket(name));
        link->pid        = pid;
        event.peer->data = link;
        m_links.push_back(link);
        m_accepted.push_back({link->connection, pid});
        for (size_t i = 0; i < m_contacts.size(); i++)
        {
            if (m_hosting && m_contacts[i].pid == pid)
            {
                m_contacts.erase(m_contacts.begin() + (long) i);
                break;
            }
        }

        // A saturated upload bloats the RTT and ENet would shrink the window to one MTU per RTT, which stalls every packet behind the chunk stream
        enet_peer_throttle_configure(event.peer, ENET_PEER_PACKET_THROTTLE_INTERVAL, ENET_PEER_PACKET_THROTTLE_SCALE, 0);
        Logger::LogInfo("Peer network connected name=%s port=%u", name.c_str(), event.peer->address.port);
        return;
    }

    Link *link = (Link *) event.peer->data;
    if (event.type == ENET_EVENT_TYPE_RECEIVE)
    {
        std::vector<uint8_t> bytes;
        if (!link || !link->connection)
        {
            Logger::LogError("Peer network dropped a packet for a removed connection pid=%u", link ? link->pid : 0);
        }
        else if (!DecodePayload(event.packet->data, event.packet->dataLength, &bytes))
        {
            Logger::LogError("Peer network could not decode a packet name=%s size=%zu", link->connection->GetName().c_str(),
                             event.packet->dataLength);
        }
        else
        {
            std::shared_ptr<Packet> packet = m_packetRegistry.ReadPacket(bytes, m_hosting);
            if (!packet)
            {
                Logger::LogError("Peer network received an unknown packet name=%s size=%zu", link->connection->GetName().c_str(), bytes.size());
            }
            else if (m_hosting)
            {
                link->connection->Send(packet);
            }
            else
            {
                link->connection->QueueIncomingPacket(packet);
            }
        }

        enet_packet_destroy(event.packet);
        return;
    }

    if (event.type != ENET_EVENT_TYPE_DISCONNECT)
    {
        return;
    }

    if (event.peer == m_hostPeer && !link)
    {
        Logger::LogError("Peer network could not connect to the host");
        m_failed = true;
    }

    if (!link)
    {
        return;
    }

    if (link->connection)
    {
        link->connection->Close(eDisconnectReason_Closed);
    }

    for (size_t i = 0; i < m_allowed.size(); i++)
    {
        if (m_allowed[i].address.host == event.peer->address.host && m_allowed[i].address.port == event.peer->address.port)
        {
            m_allowed.erase(m_allowed.begin() + (long) i);
            break;
        }
    }

    for (size_t i = 0; i < m_links.size(); i++)
    {
        if (m_links[i] == link)
        {
            m_links.erase(m_links.begin() + (long) i);
            break;
        }
    }

    Logger::LogInfo("Peer network disconnected pid=%u", link->pid);
    event.peer->data = nullptr;
    delete link;
}

void PeerNetwork::PumpLink(Link *link)
{
    Connection *connection = link->connection;
    if (!connection || link->peer->state != ENET_PEER_STATE_CONNECTED)
    {
        return;
    }

    if (!connection->GetSocket()->IsOpen())
    {
        enet_peer_disconnect_later(link->peer, 0);
        return;
    }

    connection->SetPing((int) link->peer->roundTripTime);

    // Packets stay in the connection queue while ENet still holds unsent reliable data, so the chunk stream throttles itself
    if (enet_list_empty(&link->peer->outgoingSendReliableCommands))
    {
        constexpr size_t kMaxDelayedBytes = 8 * 1024;
        size_t pulled                     = 0;
        while (pulled < s_maxPulledBytes)
        {
            std::shared_ptr<Packet> packet = m_hosting ? connection->PollReceivedPacket(pulled < kMaxDelayedBytes) : connection->PollSentPacket();
            if (!packet)
            {
                break;
            }

            std::vector<uint8_t> bytes;
            if (!m_packetRegistry.WritePacket(packet.get(), &bytes))
            {
                Logger::LogError("Peer network could not write packet name=%s packetId=%d", connection->GetName().c_str(), packet->GetId());
                continue;
            }

            SendPayload(link, bytes);
            pulled += bytes.size();
        }
    }

    TickFlush(link);
}

void PeerNetwork::TickFlush(Link *link)
{
    Connection *connection = link->connection;
    if (!connection->IsFlushing())
    {
        return;
    }

    Clock::time_point now = Clock::now();
    if (!link->flushStarted)
    {
        link->flushStarted = true;
        link->flushStart   = now;
    }

    ENetPeer *peer = link->peer;
    size_t queued  = m_hosting ? connection->GetReceivedPacketCount() : connection->GetSentPacketCount();
    bool drained   = queued == 0 && enet_list_empty(&peer->outgoingSendReliableCommands) && enet_list_empty(&peer->outgoingCommands) &&
                     enet_list_empty(&peer->sentReliableCommands);
    if (drained || now - link->flushStart > std::chrono::milliseconds(s_flushTimeoutMs))
    {
        connection->GetSocket()->Close();
    }
}

void PeerNetwork::SendPayload(Link *link, const std::vector<uint8_t> &bytes)
{
    std::vector<uint8_t> payload;
    if (bytes.size() >= s_compressThreshold)
    {
        uLongf compressedSize = compressBound((uLong) bytes.size());
        payload.resize(5 + (size_t) compressedSize);
        if (compress2(payload.data() + 5, &compressedSize, bytes.data(), (uLong) bytes.size(), Z_BEST_SPEED) == Z_OK &&
            compressedSize + 5 < bytes.size())
        {
            payload.resize(5 + (size_t) compressedSize);
            payload[0] = 1;
            payload[1] = (uint8_t) (bytes.size() >> 24);
            payload[2] = (uint8_t) (bytes.size() >> 16);
            payload[3] = (uint8_t) (bytes.size() >> 8);
            payload[4] = (uint8_t) bytes.size();
        }
        else
        {
            payload.clear();
        }
    }

    if (payload.empty())
    {
        payload.push_back(0);
        payload.insert(payload.end(), bytes.begin(), bytes.end());
    }

    enet_peer_send(link->peer, 0, enet_packet_create(payload.data(), payload.size(), ENET_PACKET_FLAG_RELIABLE));
}

bool PeerNetwork::DecodePayload(const uint8_t *data, size_t size, std::vector<uint8_t> *outPtr)
{
    if (size >= 1 && data[0] == 0)
    {
        outPtr->assign(data + 1, data + size);
        return true;
    }

    if (size < 5 || data[0] != 1)
    {
        return false;
    }

    size_t rawSize = ((size_t) data[1] << 24) | ((size_t) data[2] << 16) | ((size_t) data[3] << 8) | (size_t) data[4];
    if (rawSize == 0 || rawSize > s_maxPacketSize)
    {
        return false;
    }

    outPtr->resize(rawSize);
    uLongf destSize = (uLongf) rawSize;
    return uncompress(outPtr->data(), &destSize, data + 5, (uLong) (size - 5)) == Z_OK && destSize == rawSize;
}
