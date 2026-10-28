#include "PlatformNet.h"

#include <cstring>

#include "PlatformSockets.h"

#include "../core/Logger.h"

#ifdef _WIN32
#include <iphlpapi.h>
#else
#include <cerrno>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#endif

const PlatformNet::Handle PlatformNet::s_invalidHandle =
#ifdef _WIN32
        INVALID_SOCKET;
#else
        -1;
#endif

bool PlatformNet::Startup()
{
#ifdef _WIN32
    static WSADATA s_wsaData;
    static int s_startupResult = WSAStartup(MAKEWORD(2, 2), &s_wsaData);
    return s_startupResult == 0;
#else
    return true;
#endif
}

bool PlatformNet::IsValid(Handle handle)
{
#ifdef _WIN32
    return handle != INVALID_SOCKET;
#else
    return handle >= 0;
#endif
}

void PlatformNet::Close(Handle handle)
{
#ifdef _WIN32
    closesocket(handle);
#else
    close(handle);
#endif
}

void PlatformNet::SetNonBlocking(Handle handle, bool nonBlocking)
{
#ifdef _WIN32
    u_long mode = nonBlocking ? 1 : 0;
    ioctlsocket(handle, FIONBIO, &mode);
#else
    int flags = fcntl(handle, F_GETFL, 0);
    if (nonBlocking)
    {
        fcntl(handle, F_SETFL, flags | O_NONBLOCK);
    }
    else
    {
        fcntl(handle, F_SETFL, flags & ~O_NONBLOCK);
    }
#endif
}

void PlatformNet::SetReceiveTimeout(Handle handle, int milliseconds)
{
#ifdef _WIN32
    DWORD timeout = (DWORD) milliseconds;
    setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
#else
    timeval timeout;
    timeout.tv_sec  = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
}

void PlatformNet::SetSendTimeout(Handle handle, int milliseconds)
{
#ifdef _WIN32
    DWORD timeout = (DWORD) milliseconds;
    setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
#else
    timeval timeout;
    timeout.tv_sec  = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
}

void PlatformNet::DisableConnectionReset(Handle handle)
{
#ifdef _WIN32
    // Without this an ICMP port unreachable from a dead address makes the next UDP receive fail
    constexpr DWORD kUdpConnectionReset = _WSAIOW(IOC_VENDOR, 12);
    BOOL enable                         = FALSE;
    DWORD returned                      = 0;
    WSAIoctl(handle, kUdpConnectionReset, &enable, sizeof(enable), nullptr, 0, &returned, nullptr, nullptr);
#else
    (void) handle;
#endif
}

int PlatformNet::GetSocketError(Handle handle)
{
    int socketError  = 0;
    socklen_t length = sizeof(socketError);
    getsockopt(handle, SOL_SOCKET, SO_ERROR, (char *) &socketError, &length);
    return socketError;
}

bool PlatformNet::LastErrorIsConnectPending()
{
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EINPROGRESS;
#endif
}

int PlatformNet::WaitWritable(Handle handle, int milliseconds)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(handle, &writeSet);

    // Winsock reports a failed non-blocking connect only in the except set, without it a refused or
    // unreachable connect waits out the full timeout instead of failing immediately
    fd_set exceptSet;
    FD_ZERO(&exceptSet);
    FD_SET(handle, &exceptSet);

    timeval timeout;
    timeout.tv_sec  = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;

    return select((int) (handle + 1), nullptr, &writeSet, &exceptSet, milliseconds < 0 ? nullptr : &timeout);
}

bool PlatformNet::ResolveIpv4Address(const std::string &host, std::string *outPtr)
{
    in_addr address;
    if (inet_pton(AF_INET, host.c_str(), &address) == 1)
    {
        *outPtr = host;
        return true;
    }

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo *results = nullptr;
    int gaiResult     = getaddrinfo(host.c_str(), nullptr, &hints, &results);
    if (gaiResult != 0)
    {
#ifndef _WIN32
        Logger::LogError("getaddrinfo failed host=%s code=%d text=%s", host.c_str(), gaiResult, gai_strerror(gaiResult));
#else
        Logger::LogError("getaddrinfo failed host=%s code=%d wsa=%d", host.c_str(), gaiResult, WSAGetLastError());
#endif
        return false;
    }

    bool found = false;
    for (addrinfo *item = results; item; item = item->ai_next)
    {
        if (!item->ai_addr || item->ai_family != AF_INET)
        {
            continue;
        }

        sockaddr_in *socketAddress = (sockaddr_in *) item->ai_addr;
        char buffer[INET_ADDRSTRLEN];
        const char *text = inet_ntop(AF_INET, &socketAddress->sin_addr, buffer, sizeof(buffer));
        if (!text)
        {
            continue;
        }

        *outPtr = text;
        found   = true;
        break;
    }

    freeaddrinfo(results);
    return found;
}

std::vector<uint32_t> PlatformNet::GetLocalIpv4Addresses()
{
    std::vector<uint32_t> result;
#ifdef _WIN32
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size  = 15000;
    std::vector<uint8_t> storage(size);
    IP_ADAPTER_ADDRESSES *adapters = (IP_ADAPTER_ADDRESSES *) storage.data();
    ULONG status                   = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &size);
    if (status == ERROR_BUFFER_OVERFLOW)
    {
        storage.resize(size);
        adapters = (IP_ADAPTER_ADDRESSES *) storage.data();
        status   = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &size);
    }

    if (status != NO_ERROR)
    {
        return result;
    }

    for (IP_ADAPTER_ADDRESSES *adapter = adapters; adapter; adapter = adapter->Next)
    {
        if (adapter->OperStatus != IfOperStatusUp)
        {
            continue;
        }

        for (IP_ADAPTER_UNICAST_ADDRESS *unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
        {
            sockaddr *socketAddress = unicast->Address.lpSockaddr;
            if (socketAddress && socketAddress->sa_family == AF_INET)
            {
                result.push_back(((sockaddr_in *) socketAddress)->sin_addr.s_addr);
            }
        }
    }
#else
    ifaddrs *addresses = nullptr;
    if (getifaddrs(&addresses) != 0)
    {
        return result;
    }

    for (ifaddrs *address = addresses; address; address = address->ifa_next)
    {
        if (address->ifa_addr && address->ifa_addr->sa_family == AF_INET && (address->ifa_flags & IFF_UP))
        {
            result.push_back(((sockaddr_in *) address->ifa_addr)->sin_addr.s_addr);
        }
    }

    freeifaddrs(addresses);
#endif

    size_t i = 0;
    while (i < result.size())
    {
        uint32_t hostOrder = ntohl(result[i]);
        if ((hostOrder >> 24) == 127 || (hostOrder >> 16) == 0xA9FE)
        {
            result.erase(result.begin() + (long) i);
            continue;
        }

        i++;
    }

    return result;
}
