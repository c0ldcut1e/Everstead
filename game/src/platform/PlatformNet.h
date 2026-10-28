#pragma once

#include <cstdint>
#include <string>
#include <vector>

class PlatformNet
{
public:
#ifdef _WIN32
    typedef uintptr_t Handle;
#else
    typedef int Handle;
#endif

    static const Handle s_invalidHandle;

    static bool Startup();

    static bool IsValid(Handle handle);
    static void Close(Handle handle);

    static void SetNonBlocking(Handle handle, bool nonBlocking);
    static void SetReceiveTimeout(Handle handle, int milliseconds);
    static void SetSendTimeout(Handle handle, int milliseconds);
    static void DisableConnectionReset(Handle handle);

    static int GetSocketError(Handle handle);
    static bool LastErrorIsConnectPending();

    static int WaitWritable(Handle handle, int milliseconds);

    static bool ResolveIpv4Address(const std::string &host, std::string *outPtr);
    static std::vector<uint32_t> GetLocalIpv4Addresses();
};
