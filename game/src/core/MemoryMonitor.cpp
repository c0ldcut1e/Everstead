#include "MemoryMonitor.h"

#include <cstdlib>

#include "../platform/PlatformProcess.h"
#include "../platform/PlatformThread.h"
#include "../utils/Time.h"
#include "Logger.h"

MemoryMonitor *MemoryMonitor::GetInstance()
{
    static MemoryMonitor s_instance;
    return &s_instance;
}

MemoryMonitor::MemoryMonitor()
{
    m_lastOutOfMemoryNotifyMs = 0;
    m_lastMemoryPollMs        = 0;
    m_lastMemoryTrimMs        = 0;
    m_trimRunning             = false;
    m_limitBytes              = 0;
    m_pressure                = false;
}

MemoryMonitor::~MemoryMonitor()
{
    if (m_trimThread.joinable())
    {
        m_trimThread.join();
    }
}

void MemoryMonitor::Tick()
{
    uint32_t now = Time::GetMilliseconds();
    if (m_lastMemoryTrimMs == 0 || now - m_lastMemoryTrimMs >= s_memoryTrimIntervalMs)
    {
        m_lastMemoryTrimMs = now;
        StartTrim();
    }

    IsUnderPressure();
}

bool MemoryMonitor::ConsumeNotification()
{
    uint32_t now = Time::GetMilliseconds();
    if (m_lastOutOfMemoryNotifyMs != 0 && now - m_lastOutOfMemoryNotifyMs < s_outOfMemoryNotifyIntervalMs)
    {
        return false;
    }

    m_lastOutOfMemoryNotifyMs = now;
    return true;
}

void MemoryMonitor::ReportOutOfMemory(const char *context)
{
    m_pressure = true;
    if (!ConsumeNotification())
    {
        return;
    }

    Logger::LogError("Out of memory in %s", context ? context : "unknown");
}

uint64_t MemoryMonitor::GetLimitBytes() const
{
    return m_limitBytes;
}

bool MemoryMonitor::IsUnderPressure()
{
    std::lock_guard<std::mutex> lock(m_pressureMutex);
    uint32_t now = Time::GetMilliseconds();
    if (m_lastMemoryPollMs != 0 && now - m_lastMemoryPollMs < s_memoryPollIntervalMs)
    {
        return m_pressure;
    }

    m_lastMemoryPollMs = now;

    uint64_t totalBytes     = 0;
    uint64_t availableBytes = 0;
    bool systemLow          = PlatformProcess::GetSystemMemoryBytes(&totalBytes, &availableBytes) && availableBytes < GetLowMemoryBytes(totalBytes);
    m_pressure              = systemLow || IsOverBudget(s_memoryPressurePercent);

    if (m_pressure)
    {
        StartTrim();
    }

    return m_pressure;
}

bool MemoryMonitor::HasHeadroom() const
{
    uint64_t totalBytes     = 0;
    uint64_t availableBytes = 0;
    if (PlatformProcess::GetSystemMemoryBytes(&totalBytes, &availableBytes) && availableBytes < GetLowMemoryBytes(totalBytes) * 2)
    {
        return false;
    }

    return !IsOverBudget(s_memoryHeadroomPercent);
}

void MemoryMonitor::ApplyLimit(uint64_t limitMb)
{
    const char *memoryEnv = getenv("EVERSTEAD_MEMORY");
    if (memoryEnv)
    {
        long long requested = atoll(memoryEnv);
        if (requested > 0)
        {
            limitMb = (uint64_t) requested;
        }
        else
        {
            Logger::LogWarn("Ignoring invalid EVERSTEAD_MEMORY value \"%s\"", memoryEnv);
        }
    }

    // We enforce this ourselves because an OS cap on Windows also counts driver memory and fails allocations early
    m_limitBytes = limitMb * 1024ULL * 1024ULL;
    Logger::LogInfo("Memory limit set to %llu MB", (unsigned long long) limitMb);
}

uint64_t MemoryMonitor::GetLowMemoryBytes(uint64_t totalBytes) const
{
    uint64_t scaled = totalBytes / 100 * (uint64_t) s_lowMemoryPercent;
    return scaled > s_lowMemoryFloorBytes ? scaled : s_lowMemoryFloorBytes;
}

bool MemoryMonitor::IsOverBudget(int percent) const
{
    uint64_t usedBytes = 0;
    if (m_limitBytes == 0 || !PlatformProcess::GetPrivateMemoryBytes(&usedBytes))
    {
        return false;
    }

    return usedBytes >= m_limitBytes / 100 * (uint64_t) percent;
}

void MemoryMonitor::StartTrim()
{
    std::lock_guard<std::mutex> lock(m_trimMutex);
    if (m_trimRunning.load())
    {
        return;
    }

    if (m_trimThread.joinable())
    {
        m_trimThread.join();
    }

    m_trimRunning = true;
    m_trimThread  = std::thread(
            [this]
            {
                PlatformThread::LowerCurrentPriority(false);
                PlatformProcess::TrimMemory();
                m_trimRunning = false;
            });
}
