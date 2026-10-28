#pragma once

#include <atomic>
#include <cstdint>

class CrashHandler
{
public:
    static void Install(bool alwaysEnabled);
    static void Heartbeat();
    static void SetContext(const char *fmt, ...);
    static void ClearContext();

private:
    static void WatchdogLoop();

    static bool s_enabled;
    static std::atomic<uint64_t> s_lastHeartbeatMs;
};
