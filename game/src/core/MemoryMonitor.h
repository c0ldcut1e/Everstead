#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

class MemoryMonitor
{
public:
    static MemoryMonitor *GetInstance();

    MemoryMonitor();
    ~MemoryMonitor();

    void ApplyLimit(uint64_t limitMb);
    void Tick();
    void ReportOutOfMemory(const char *context);
    bool ConsumeNotification();
    bool IsUnderPressure();
    bool HasHeadroom() const;
    uint64_t GetLimitBytes() const;

    // The limit applies to the game's own heap, pressure also kicks in when the whole system runs low
    static const uint64_t s_defaultMemoryLimitMb = 2048;

private:
    static const uint64_t s_lowMemoryFloorBytes         = 768ULL * 1024ULL * 1024ULL;
    static const int s_lowMemoryPercent                 = 6;
    static const uint32_t s_outOfMemoryNotifyIntervalMs = 10000;
    static const uint32_t s_memoryPollIntervalMs        = 250;
    static const uint32_t s_memoryTrimIntervalMs        = 5000;
    static const int s_memoryPressurePercent            = 80;
    static const int s_memoryHeadroomPercent            = 65;

    void StartTrim();
    uint64_t GetLowMemoryBytes(uint64_t totalBytes) const;
    bool IsOverBudget(int percent) const;

    uint32_t m_lastOutOfMemoryNotifyMs;
    uint32_t m_lastMemoryPollMs;
    uint32_t m_lastMemoryTrimMs;
    std::thread m_trimThread;
    std::mutex m_trimMutex;
    std::mutex m_pressureMutex;
    std::atomic<bool> m_trimRunning;
    uint64_t m_limitBytes;
    bool m_pressure;
};
