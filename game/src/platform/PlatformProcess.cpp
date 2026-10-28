#include "PlatformProcess.h"

#include <mimalloc.h>

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <malloc.h>
#include <psapi.h>
#else
#include <malloc.h>
#include <unistd.h>
#endif

#include "../memory/HeapCounter.h"

bool PlatformProcess::GetResidentMemoryBytes(uint64_t *outPtr)
{
    if (!outPtr)
    {
        return false;
    }

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters;
    memset(&counters, 0, sizeof(counters));
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        return false;
    }

    *outPtr = (uint64_t) counters.WorkingSetSize;
    return true;
#else
    FILE *file = fopen("/proc/self/statm", "r");
    if (!file)
    {
        return false;
    }

    unsigned long totalPages    = 0;
    unsigned long residentPages = 0;
    int fields                  = fscanf(file, "%lu %lu", &totalPages, &residentPages);
    fclose(file);
    if (fields != 2)
    {
        return false;
    }

    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0)
    {
        return false;
    }

    *outPtr = (uint64_t) residentPages * (uint64_t) pageSize;
    return true;
#endif
}

bool PlatformProcess::GetSystemMemoryBytes(uint64_t *totalPtr, uint64_t *availablePtr)
{
    if (!totalPtr || !availablePtr)
    {
        return false;
    }

#ifdef _WIN32
    MEMORYSTATUSEX status;
    memset(&status, 0, sizeof(status));
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status))
    {
        return false;
    }

    // Allocations fail once either physical memory or the system commit limit runs out
    *totalPtr     = (uint64_t) status.ullTotalPhys;
    *availablePtr = status.ullAvailPhys < status.ullAvailPageFile ? (uint64_t) status.ullAvailPhys : (uint64_t) status.ullAvailPageFile;
    return true;
#else
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file)
    {
        return false;
    }

    char line[256];
    unsigned long long totalKib     = 0;
    unsigned long long availableKib = 0;
    bool foundTotal                 = false;
    bool foundAvailable             = false;
    while (fgets(line, sizeof(line), file) && !(foundTotal && foundAvailable))
    {
        if (sscanf(line, "MemTotal: %llu kB", &totalKib) == 1)
        {
            foundTotal = true;
        }
        else if (sscanf(line, "MemAvailable: %llu kB", &availableKib) == 1)
        {
            foundAvailable = true;
        }
    }

    fclose(file);
    if (!foundTotal || !foundAvailable)
    {
        return false;
    }

    *totalPtr     = (uint64_t) totalKib * 1024ULL;
    *availablePtr = (uint64_t) availableKib * 1024ULL;
    return true;
#endif
}

bool PlatformProcess::GetPrivateMemoryBytes(uint64_t *outPtr)
{
    if (!outPtr)
    {
        return false;
    }

    // Windows commit includes driver copies of GPU buffers and Linux RSS doesn't, so only our own heap is comparable
    *outPtr = HeapCounter::GetBytes();
    return true;
}

bool PlatformProcess::GetProcessCommitBytes(uint64_t *outPtr)
{
    if (!outPtr)
    {
        return false;
    }

#ifdef _WIN32
    // PagefileUsage is the same commit charge as PrivateUsage, which Wine never fills in
    PROCESS_MEMORY_COUNTERS counters;
    memset(&counters, 0, sizeof(counters));
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        return false;
    }

    *outPtr = (uint64_t) counters.PagefileUsage;
    return true;
#else
    FILE *file = fopen("/proc/self/status", "r");
    if (!file)
    {
        return false;
    }

    char line[256];
    bool found = false;
    while (fgets(line, sizeof(line), file))
    {
        unsigned long long kilobytes = 0;
        if (sscanf(line, "RssAnon: %llu kB", &kilobytes) == 1)
        {
            *outPtr = (uint64_t) kilobytes * 1024ULL;
            found   = true;
            break;
        }
    }

    fclose(file);
    return found;
#endif
}

void PlatformProcess::ConfigureAllocator()
{
    mi_option_set(mi_option_arena_reserve, s_allocatorArenaReserveKib);
    // Eager commit makes Windows count untouched segments that Linux hides, so pages commit on first use everywhere
    mi_option_set(mi_option_eager_commit, 0);
    mi_option_set(mi_option_arena_eager_commit, 0);
#ifndef _WIN32
    mallopt(M_ARENA_MAX, s_maxAllocatorArenas);
    mallopt(M_TRIM_THRESHOLD, s_allocatorTrimThresholdBytes);
#endif
}

void PlatformProcess::TrimMemory()
{
    mi_collect(true);
#ifndef _WIN32
    malloc_trim(0);
#endif
}
