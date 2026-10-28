#pragma once

#include <cstdint>

class PlatformProcess
{
public:
    static const int s_otherBucketCount = 4;

    static const int s_maxAllocatorArenas          = 2;
    static const int s_allocatorTrimThresholdBytes = 4 * 1024 * 1024;
    static const long s_allocatorArenaReserveKib   = 128 * 1024;

    static bool GetResidentMemoryBytes(uint64_t *outPtr);
    static bool GetSystemMemoryBytes(uint64_t *totalPtr, uint64_t *availablePtr);
    static bool GetPrivateMemoryBytes(uint64_t *outPtr);
    static bool GetProcessCommitBytes(uint64_t *outPtr);
    static void ConfigureAllocator();
    static void TrimMemory();
};
